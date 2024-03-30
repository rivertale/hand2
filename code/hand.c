#include "hand.h"
#include "hand_buffer.c"
#include "hand_config.c"
#include "hand_curl.c"
#include "hand_git2.c"

static void
invite_students(char *path, char *github_token, char *organization, char *team)
{
    write_log("[invite students] path='%s', org='%s', team='%s'", path, organization, team);

    StringArray students = read_string_array_file(path);
    if(students.count > 0)
    {
        write_output("Retrieving existing members...");
        StringArray existing_students = retrieve_users_in_team(github_token, organization, team);
        write_output("Retrieving existing invitations...");
        StringArray existing_invitations = retrieve_existing_invitations(github_token, organization, team);

        write_log("existing member count: %d", existing_students.count);
        write_log("existing invitation count: %d", existing_invitations.count);

        int invalid_count = 0;
        for(int i = 0; i < students.count; ++i)
        {
            for(int j = 0; j < existing_students.count; ++j)
            {
                if(compare_case_insensitive(students.elem[i], existing_students.elem[j]))
                {
                    students.elem[i] = 0;
                    break;
                }
            }

            if(!students.elem[i]) continue;
            for(int j = 0; j < existing_invitations.count; ++j)
            {
                if(compare_case_insensitive(students.elem[i], existing_invitations.elem[j]))
                {
                    students.elem[i] = 0;
                    break;
                }
            }

            if(!students.elem[i]) continue;
            if(!github_user_exists(github_token, students.elem[i]))
            {
                write_error("github user '%s' does not exist", students.elem[i]);
                ++invalid_count;
                students.elem[i] = 0;
                break;
            }
        }

        if(invalid_count == 0)
        {
            int failure_count = 0;
            int success_count = 0;
            write_output("Inviting new members...");
            for(int i = 0; i < students.count; ++i)
            {
                if(!students.elem[i]) continue;
                if(invite_user_to_team(github_token, students.elem[i], organization, team))
                {
                    ++success_count;
                    write_output("Invite '%s' to '%s/%s'", students.elem[i], organization, team);
                }
                else
                {
                    ++failure_count;
                    write_error("Can't invite '%s' to '%s/%s'", students.elem[i], organization, team);
                }
            }

            write_output("");
            write_output("[Summary]");
            write_output("    Total students: %d", students.count);
            write_output("    New invitation (success): %d", success_count);
            write_output("    New invitation (failure): %d", failure_count);
            write_output("    Existing members: %d", existing_students.count);
            write_output("    Existing invitations: %d", existing_invitations.count);
        }
        else
        {
            write_output("ABORT: some usernames are invalid");
        }
        free_string_array(&existing_invitations);
        free_string_array(&existing_students);
    }
    else
    {
        write_error("input file '%s' is empty", path);
    }
    free_string_array(&students);
}

static void
collect_homework(char *title, char *out_path, time_t deadline, time_t cutoff, int penalty_per_day, int is_weekends_one_day,
                 char *github_token, char *organization, char *google_token, char *spreadsheet_id, char *student_key)
{
    tm t0 = calendar_time(deadline);
    tm t1 = calendar_time(cutoff);
    write_log("[collect homework] title='%s', organization='%s', "
              "deadline='%d-%02d-%02d_%02d:%02d:%02d', cutoff='%d-%02d-%02d_%02d:%02d:%02d'",
              title, organization,
              t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec,
              t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

    FILE *out_file = fopen(out_path, "wb");
    if(!out_file)
    {
        write_error("unable to create '%s'", out_path);
        return;
    }

    write_output("Retrieving repos with prefix '%s'...", title);
    StringArray repos = retrieve_repos_by_prefix(github_token, organization, title);

    write_output("Retrieving default branches...");
    StringArray branches = retrieve_default_branches(&repos, github_token, organization);
    assert(branches.count == repos.count);

    write_output("Retrieving pushes before deadline...");
    GitCommitHash *hash = (GitCommitHash *)allocate_memory(repos.count * sizeof(*hash));
    time_t *push_time = (time_t *)allocate_memory(repos.count * sizeof(*push_time));
    retrieve_pushes_before_cutoff(hash, push_time, &repos, &branches, github_token, organization, cutoff);

    // NOTE: for the weekends-count-as-1-day policy, the examples use the time format 0:00:00 ~ 23:59:59.
    // 1. if the late submission starts at 11:59:59 (Fri.):
    //    (1) submit at 11:59:59 (Sat.) is counted as 1-day, hour  0-24 are treated as Friday.
    //    (2) submit at 11:59:59 (Sun.) is counted as 2-day, hour 24-48 are treated as Saturday.
    //    (3) submit at 11:59:59 (Mon.) is counted as 2-day, hour 48-72 are treated as Sunday.
    // 2. if the late submission starts at 12:00:00 (Fri.):
    //    (1) submit at 12:00:00 (Sat.) is counted as 1-day, hour  0-24 are treated as Saturday.
    //    (2) submit at 12:00:00 (Sun.) is counted as 1-day, hour 24-48 are treated as Sunday.
    //    (3) submit at 12:00:00 (Mon.) is counted as 2-day, hour 48-72 are treated as Monday.
    int submission_count = 0;
    int late_submission_count = 0;
    int start_weekday = (t0.tm_hour < 12) ? t0.tm_wday : (t0.tm_wday + 1) % 7;

    Sheet sheet = retrieve_sheet(google_token, spreadsheet_id, title);
    int student_x = find_key_index(&sheet, student_key);
    for(int y = 0; y < sheet.height; ++y)
    {
        int index = -1;
        static char req_repo[256];
        char *student = get_value(&sheet, student_x, y);
        if(format_string(req_repo, sizeof(req_repo), "%s-%s", title, student))
        {
            index = find_index_case_insensitive(&repos, req_repo);
        }

        int delay = 0;
        if(index != -1)
        {
            ++submission_count;
            if(push_time[index] > deadline)
            {
                ++late_submission_count;
                delay = (int)((push_time[index] - deadline + 86399) / 86400);
                assert(delay > 0);
                if(is_weekends_one_day)
                {
                    int weekend_before_deadline = (start_weekday + 6) / 7 + start_weekday / 7;
                    int weekend_before_cutoff = (start_weekday + delay + 6) / 7 + (start_weekday + delay) / 7;
                    int weekend_day_count = weekend_before_cutoff - weekend_before_deadline;
                    delay -= (weekend_day_count / 2);
                }

                tm t = calendar_time(push_time[index]);
                if(late_submission_count == 0)
                {
                    write_output("");
                    write_output("[Late submission]");
                    write_output("%3s  %12s  %-19s  %-24s  %-40s",
                                 "#", "delay (days)", "push_time", "repository", "hash");
                }
                write_output("%3d  %12d  %4d-%02d-%02d_%02d:%02d:%02d  %-24s  %-40s",
                             late_submission_count, delay,
                             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                             repos.elem[index], hash[index].trim);
            }
        }
        fprintf(out_file, "%d%%\n", max(100 - penalty_per_day * delay, 0));
    }

    write_output("");
    write_output("[Summary]");
    write_output("    Total student: %d", sheet.height);
    write_output("    Total submission: %d", submission_count);
    write_output("    Late submission: %d", late_submission_count);
    write_output("    Deadline: %d-%02d-%02d %02d:%02d:%02d",
                 t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec);
    write_output("    Cutoff: %d-%02d-%02d %02d:%02d:%02d",
                 t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);
    fclose(out_file);

    free_memory(push_time);
    free_memory(hash);
    free_string_array(&branches);
    free_string_array(&repos);
}

static int
format_report_by_file_replacement(GrowableBuffer *out, GrowableBuffer *format, char *dir)
{
    *out = allocate_growable_buffer();

    int success = 1;
    int depth = 0;
    char *identifier = 0;
    for(size_t i = 0; i < format->used; ++i)
    {
        char *c = format->memory + i;
        if(c[0] == '$' && c[1] == '$')
        {
            write_constant_string(out, "$$");
            ++i;
        }
        else if(c[0] == '$' && c[1] == '{')
        {
            if(depth++ == 0) { identifier = c + 2; }
            ++i;
        }
        else if(c[0] == '}')
        {
            depth = max(depth - 1, 0);
            if(depth == 0)
            {
                *c = 0;
                if(identifier[0] == '.' && identifier[1] == '/')
                {
                    static char path[MAX_PATH_LEN];
                    if(format_string(path, MAX_PATH_LEN, "%s/%s", dir, identifier))
                    {
                        GrowableBuffer replacement = read_entire_file(path);
                        write_growable_buffer(out, replacement.memory, replacement.used);
                        free_growable_buffer(&replacement);
                    }
                    else
                    {
                        success = 0;
                    }
                }
                else
                {
                    write_constant_string(out, "${");
                    write_growable_buffer(out, identifier, string_len(identifier));
                    write_constant_string(out, "}");
                }
                *c = '}';
            }
        }
        else if(depth == 0)
        {
            write_growable_buffer(out, c, 1);
        }
    }
    return success;
}

static void
grade_homework_on_progress(int index, int count, char *work_dir)
{
    write_output("[%d/%d] Grading '%s'", index + 1, count, work_dir);
}

static void
grade_homework_on_complete(int index, int count, int exit_code, char *work_dir, char *stdout_content, char *stderr_content)
{
    (void)stdout_content;
    if(exit_code != 0)
    {
        write_output("[%d/%d] Fail '%s'\n%s", index + 1, count, work_dir, stderr_content);
    }
}

static void
grade_homework(char *title, char *out_path, time_t deadline, time_t cutoff, char *template_repo, char *template_branch, char *feedback_repo,
               char *command, char *score_relative_path, int thread_count, int should_match_title,
               char *github_token, char *organization, char *google_token, char *spreadsheet_id, char *student_key, char *id_key)
{
    tm t0 = calendar_time(deadline);
    tm t1 = calendar_time(cutoff);
    write_log("[grade homework] title='%s', out_path='%s', "
              "deadline='%d-%02d-%02d_%02d:%02d:%02d', cutoff='%d-%02d-%02d_%02d:%02d:%02d', "
              "template_repo='%s', feedback_repo='%s', command='%s', score_relative_path='%s', "
              "thread_count='%d', should_match_title='%d', organization='%s', spreadsheet_id='%s', student_key='%s', id_key='%s'",
              title, out_path,
              t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec,
              t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec,
              template_repo, feedback_repo, command, score_relative_path, thread_count, should_match_title,
              organization, spreadsheet_id, student_key, id_key);

    static char username[MAX_URL_LEN];
    static char template_dir[MAX_PATH_LEN];
    static char feedback_dir[MAX_PATH_LEN];
    static char template_test_dir[MAX_PATH_LEN];
    static char template_docker_dir[MAX_PATH_LEN];
    static char feedback_report_dir[MAX_PATH_LEN];
    static char feedback_homework_dir[MAX_PATH_LEN];
    static char report_template_path[MAX_PATH_LEN];

    GrowableBuffer report_template = {0};
    StringArray repos = {0};
    FILE *out_file = fopen(out_path, "wb");
    if(!out_file)
    {
        write_error("unable to create '%s'", out_path);
        return;
    }

    if(!retrieve_username(username, MAX_URL_LEN, github_token))
    {
        write_error("unable to retrieve username");
        return;
    }

    // TODO: handle cache_repository error
    write_output("Retrieving homework template...");
    GitCommitHash template_hash = retrieve_latest_commit(github_token, organization, template_repo, template_branch);
    cache_repository(template_dir, MAX_PATH_LEN, username, github_token, organization, template_repo, &template_hash);

    write_output("Retrieving feedback repository...");
    cache_repository(feedback_dir, MAX_PATH_LEN, username, github_token, organization, feedback_repo, 0);

    if(!format_string(template_test_dir, MAX_PATH_LEN, "%s/test", template_dir) ||
       !format_string(template_docker_dir, MAX_PATH_LEN, "%s/docker", template_dir) ||
       !format_string(feedback_homework_dir, MAX_PATH_LEN, "%s/%s",feedback_dir, title) ||
       !format_string(feedback_report_dir, MAX_PATH_LEN, "%s/%s/reports",feedback_dir, title) ||
       !format_string(report_template_path, MAX_PATH_LEN, "%s/report_template_%s.md", feedback_dir, title))
    {
        return;
    }

    platform.delete_directory(feedback_report_dir);
    platform.create_directory(feedback_homework_dir);
    platform.create_directory(feedback_report_dir);
    report_template = read_entire_file(report_template_path);

    if(should_match_title)
    {
        repos = allocate_string_array();
        append_string_array(&repos, title);
    }
    else
    {
        write_output("Retrieving repos with prefix '%s'...", title);
        repos = retrieve_repos_by_prefix(github_token, organization, title);
    }

    write_output("Retrieving default branches...");
    StringArray branches = retrieve_default_branches(&repos, github_token, organization);
    assert(branches.count == repos.count);

    write_output("Retrieving pushes before deadline...");
    GitCommitHash *hash = (GitCommitHash *)allocate_memory(repos.count * sizeof(*hash));
    time_t *push_time = (time_t *)allocate_memory(repos.count * sizeof(*push_time));
    retrieve_pushes_before_cutoff(hash, push_time, &repos, &branches, github_token, organization, cutoff);

    write_output("Retrieving student repositories...");
    Work *works = (Work *)allocate_memory(repos.count * sizeof(*works));
    clear_memory(works, repos.count * sizeof(*works));
    for(int i = 0; i < repos.count; ++i)
    {
        Work *work = works + i;
        static char dir[MAX_PATH_LEN];
        static char test_dir[MAX_PATH_LEN];
        static char docker_dir[MAX_PATH_LEN];
        write_output("Retrieving homework from '%s'", repos.elem[i]);
        if(cache_repository(dir, MAX_PATH_LEN, username, github_token, organization, repos.elem[i], &hash[i]) &&
           format_string(test_dir, MAX_PATH_LEN, "%s/test", dir) &&
           format_string(docker_dir, MAX_PATH_LEN, "%s/docker", dir))
        {
            // IMPORTANT: After mounting a volume for the first time on wsl with "docker run -v", change the
            // volume's content seems to mess up the ".." inode that navigates to /mnt
            //
            // For example, after deleting and copying the modified files into the volume, the inode
            // "../../.." that navigate to /mnt become the same as "../../."
            //
            // I don't know why, I don't want to know why, I shouldn't wonder why, just don't use WSL
            // to directly launch hand2

            // NOTE: in case we want to modify cache/tmpl-hwX locally after cloning students' repository,
            // we update the directories every time instead of updating it only when we clone.
            platform.delete_directory(test_dir);
            platform.delete_directory(docker_dir);
            platform.copy_directory(test_dir, template_test_dir);
            platform.copy_directory(docker_dir, template_docker_dir);
            format_string(work->command, sizeof(work->command), "%s", command);
            format_string(work->work_dir, sizeof(work->work_dir), "%s", dir);
            format_string(work->stdout_path, sizeof(work->stdout_path),
                          "%s/%s_%s_stdout.log", g_log_dir, repos.elem[i], hash[i].trim);
            format_string(work->stderr_path, sizeof(work->stderr_path),
                          "%s/%s_%s_stderr.log", g_log_dir, repos.elem[i], hash[i].trim);
        }
        else
        {
            // TODO: error
        }
    }

    int submission_count = 0;
    int late_submission_count = 0;
    int failure_count = 0;
    write_output("Start grading...");
    platform.wait_for_completion(thread_count, repos.count, works, grade_homework_on_progress, grade_homework_on_complete);

    write_output("Generating report...");
    Sheet sheet = retrieve_sheet(google_token, spreadsheet_id, title);
    int student_x = find_key_index(&sheet, student_key);
    int id_x = find_key_index(&sheet, id_key);
    for(int y = 0; y < sheet.height; ++y)
    {
        int index = -1;
        static char req_name[256];
        char *student = get_value(&sheet, student_x, y);
        char *student_id = get_value(&sheet, id_x, y);
        if(format_string(req_name, sizeof(req_name), "%s-%s", title, student))
        {
            index = find_index_case_insensitive(&repos, req_name);
        }

        double score = 0;
        if(index != -1)
        {
            ++submission_count;
            if(push_time[index] > deadline) { ++late_submission_count; }
            if(works[index].exit_code != 0) { ++failure_count; }

            static char report_path[MAX_PATH_LEN];
            static char score_path[MAX_PATH_LEN];
            if(works[index].exit_code == 0 &&
               format_string(report_path, MAX_PATH_LEN, "%s/%s.md", feedback_report_dir, student_id) &&
               format_string(score_path, MAX_PATH_LEN, "%s/%s", works[index].work_dir, score_relative_path))
            {
                GrowableBuffer score_content = read_entire_file(score_path);
                for(char *c = score_content.memory; *c; ++c)
                {
                    if('0' <= *c && *c <= '9')
                    {
                        score = atof(c);
                        break;
                    }
                }
                free_growable_buffer(&score_content);

                GrowableBuffer report;
                if(format_report_by_file_replacement(&report, &report_template, works[index].work_dir))
                {
                    FILE *report_file = fopen(report_path, "wb");
                    if(report_file)
                    {
                        fwrite(report.memory, report.used, 1, report_file);
                        fclose(report_file);
                    }
                }
                else
                {
                    // TODO: error
                }
            }
        }
        fprintf(out_file, "%f\n", score);
    }
    write_output("");
    write_output("[Summary]");
    write_output("    Total student: %d", sheet.height);
    write_output("    Total submission: %d", submission_count);
    write_output("    Late submission: %d", late_submission_count);
    write_output("    Failed submission: %d", failure_count);
    write_output("    Deadline: %d-%02d-%02d %02d:%02d:%02d",
                 t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec);
    write_output("    Cutoff: %d-%02d-%02d %02d:%02d:%02d",
                 t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);
    write_output("NOTE: reports generated at '%s', remember to push the reports", feedback_report_dir);
    fclose(out_file);
}

static int
format_feedback_issues(StringArray *out, StringArray *format, Sheet *sheet)
{
    int success = 1;
    *out = allocate_string_array();
    assert(format->count == sheet->height);

    GrowableBuffer buffer = allocate_growable_buffer();
    for(int y = 0; y < sheet->height; ++y)
    {
        clear_growable_buffer(&buffer);
        int depth = 0;
        char *identifier = 0;
        for(char *c = format->elem[y]; *c; ++c)
        {
            if(c[0] == '$' && c[1] == '$')
            {
                write_constant_string(&buffer, "$");
                ++c;
            }
            else if(c[0] == '$' && c[1] == '{')
            {
                if(depth++ == 0) { identifier = c + 2; }
                ++c;
            }
            else if(c[0] == '}')
            {
                if(depth == 1)
                {
                    *c = 0;
                    int x = find_key_index(sheet, identifier);
                    *c = '}';
                    if(x >= 0)
                    {
                        char *replacement = get_value(sheet, x, y);
                        write_growable_buffer(&buffer, replacement, string_len(replacement));
                    }
                    else
                    {
                        // TODO: error
                        success = 0;
                    }
                }
                depth = max(depth - 1, 0);
            }
            else if(depth == 0)
            {
                write_growable_buffer(&buffer, c, 1);
            }
        }
        append_string_array(out, buffer.memory);
    }
    free_growable_buffer(&buffer);

    assert(out->count == sheet->height);
    if(!success)
    {
        free_string_array(out);
    }
    return success;
}

static void
announce_grade(char *title, char *feedback_repo,
               char *github_token, char *organization, char *google_token, char *spreadsheet_id, char *student_key, char *id_key)
{
    write_log("[announce grade] title='%s', feedback_repo='%s', spreadsheet_id='%s', student_key='%s', organization='%s'",
              title, feedback_repo, spreadsheet_id, student_key, organization);

    static char issue_title[256];
    static char username[MAX_URL_LEN];
    static char feedback_dir[MAX_PATH_LEN];
    static char feedback_report_dir[MAX_PATH_LEN];

    if(!retrieve_username(username, MAX_URL_LEN, github_token))
    {
        write_error("unable to retrieve username");
        return;
    }

    write_output("Retrieving feedback repository...");
    cache_repository(feedback_dir, MAX_PATH_LEN, username, github_token, organization, feedback_repo, 0);

    if(!format_string(issue_title, sizeof(issue_title), "Grade for %s", title) ||
       !format_string(feedback_report_dir, MAX_PATH_LEN, "%s/%s/reports", feedback_dir, title))
    {
        return;
    }

    write_output("Retrieving repos with prefix '%s'...", title);
    StringArray repos = retrieve_repos_by_prefix(github_token, organization, title);

    write_output("Retrieving issue '%s'...", issue_title);
    int *issue_numbers = (int *)allocate_memory(repos.count * sizeof(*issue_numbers));
    retrieve_issue_numbers_by_title(issue_numbers, &repos, github_token, organization, issue_title);

    int announcement_count = 0;
    int failure_count = 0;
    Sheet sheet = retrieve_sheet(google_token, spreadsheet_id, title);
    if(sheet.width && sheet.height)
    {
        int student_x = find_key_index(&sheet, student_key);
        int id_x = find_key_index(&sheet, id_key);
        if(student_x >= 0 && id_x >= 0)
        {
            StringArray reports = allocate_string_array();
            for(int y = 0; y < sheet.height; ++y)
            {
                char *student_id = get_value(&sheet, id_x, y);
                static char report_path[MAX_PATH_LEN];
                format_string(report_path, MAX_PATH_LEN, "%s/%s.md", feedback_report_dir, student_id);
                GrowableBuffer content = read_entire_file(report_path);
                append_string_array(&reports, content.memory);
            }

            StringArray issue_bodies;
            if(format_feedback_issues(&issue_bodies, &reports, &sheet))
            {
                for(int y = 0; y < sheet.height; ++y)
                {
                    int index = -1;
                    static char req_repo[256];
                    char *student = get_value(&sheet, student_x, y);
                    if(format_string(req_repo, sizeof(req_repo), "%s-%s", title, student))
                    {
                        index = find_index_case_insensitive(&repos, req_repo);
                    }

                    if(index != -1)
                    {
                        write_output("Announce grade for '%s'", repos.elem[index]);
                        ++announcement_count;
                        if(issue_numbers[index])
                        {
                            if(!edit_issue(github_token, organization, repos.elem[index], issue_title, issue_bodies.elem[y], issue_numbers[index]))
                            {
                                ++failure_count;
                            }
                        }
                        else
                        {
                            if(!create_issue(github_token, organization, repos.elem[index], issue_title, issue_bodies.elem[y]))
                            {
                                ++failure_count;
                            }
                        }
                        // NOTE: to bypass the github rate limit
                        // TODO: need further investigation on the error messages github returns
                        sleep(1);
                    }
                }
            }
            else
            {

            }

            write_output("");
            write_output("[Summary]");
            write_output("    Total students: %d", sheet.height);
            write_output("    Total announcement: %d", announcement_count);
            write_output("    Failed announcement: %d", failure_count);
        }
        else
        {
            if(student_x < 0) write_error("Key '%s' not found", student_key);
            if(id_x < 0) write_error("Key '%s' not found", id_key);
        }
    }
    else
    {
        write_error("Sheet '%s' is empty under spreadsheet id '%s'", title, spreadsheet_id);
    }
    free_sheet(&sheet);
}

static void
eval(ArgParser *parser, Config *config)
{
    int show_usage = 0;
    for(;;)
    {
        char *option = next_option(parser);
        if(!option) break;

        if(compare_string(option, "--help"))
        {
            show_usage = 1;
        }
        else if(compare_string(option, "--log"))
        {
            struct tm time = current_calendar_time();
            static char path[MAX_PATH_LEN];
            if(format_string(path, MAX_PATH_LEN, "%s/%d-%02d-%02d_%02d-%02d-%02d.log",
                             g_log_dir, time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                             time.tm_hour, time.tm_min, time.tm_sec))
            {
                platform.create_directory(g_log_dir);
                g_log_file = fopen(path, "wb");
            }
            else
            {
                // TODO: log
            }
        }
        else
        {
            show_usage = 1;
            write_error("unknown option '%s'", option);
        }
    }

    char *command = next_arg(parser);
    // NOTE: we assume command is not null after this check
    if(!command) { show_usage = 1; }

    if(show_usage)
    {
        char *usage =
            "usage: hand2 [--options] ... command [--command-options] ... [args] ..."   "\n"
            "[options]"                                                                 "\n"
            "    --help    show this message"                                           "\n"
            "    --log     log to YYYY-MM-DD_hh-mm-ss.log"                              "\n"
            "[command]"                                                                 "\n"
            "    invite-students    invite student into github organization"            "\n"
            "    collect-homework   collect homework and retrieve late submission info" "\n"
            "    grade-homework     grade homework"                                     "\n"
            "    announce-grade     announce grade"                                     "\n"
            "[common-command-options]"                                                  "\n"
            "    --help             show the help message regards to the command"       "\n";
        write_output(usage);
        return;
    }

    if(compare_string(command, "invite-students"))
    {
        int show_command_usage = 0;
        for(char *option = next_option(parser); option; option = next_option(parser))
        {
            if(compare_string(option, "--help")) { show_command_usage = 1; }
            else
            {
                show_command_usage = 1;
                write_error("unknown option '%s'", option);
            }
        }

        char *input_path = next_arg(parser);
        if(!input_path || show_command_usage)
        {
            char *usage =
                "usage: hand2 invite-students [--command-options] ... path" "\n"
                                                                            "\n"
                "[arguments]"                                               "\n"
                "    path    file path of listed github handle to invite"   "\n"
                "[command-options]"                                         "\n";
            write_output(usage);
            return;
        }
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        char *team = config->value[Config_student_team];
        invite_students(input_path, github_token, organization, team);
    }
    else if(compare_string(command, "collect-homework"))
    {
        int show_command_usage = 0;
        int is_weekends_one_day = 1;
        for(char *option = next_option(parser); option; option = next_option(parser))
        {
            if(compare_string(option, "--help")) { show_command_usage = 1; }
            else if(compare_string(option, "--no-weekends")) { is_weekends_one_day = 0; }
            else
            {
                show_command_usage = 1;
                write_error("unknown option '%s'", option);
            }
        }

        char *title = next_arg(parser);
        char *in_time = next_arg(parser);
        char *cutoff_time = next_arg(parser);
        char *out_path = next_arg(parser);
        if(!title || !in_time || !cutoff_time || show_command_usage)
        {
            char *usage =
                "usage: hand2 collect-homework [--command-option] ... title deadline cutoff_time out_path"  "\n"
                                                                                                            "\n"
                "[arguments]"                                                                               "\n"
                "    title          title of the homework"                                                  "\n"
                "    deadline       deadline of the homework in YYYY-MM-DD-hh-mm-ss"                        "\n"
                "    cutoff_time    max late submission time after deadline (in days)"                      "\n"
                "    out_path       output file that lists penalty for all students (in the same order of"  "\n"
                "                   the spreadsheet)"                                                       "\n"
                "[command-options]"                                                                         "\n"
                "    --no-weekends  weekends are not considered as one day"                                 "\n";
            write_output(usage);
            return;
        }
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        char *google_token = config->value[Config_google_token];
        char *spreadsheet_id = config->value[Config_spreadsheet];
        char *student_key = config->value[Config_key_username];
        int penalty_per_day = atoi(config->value[Config_penalty_per_day]);
        time_t deadline = parse_time(in_time, TIME_ZONE_UTC8);
        time_t cutoff = deadline + atoi(cutoff_time) * 86400;
        collect_homework(title, out_path, deadline, cutoff, penalty_per_day, is_weekends_one_day,
                         github_token, organization, google_token, spreadsheet_id, student_key);
    }
    else if(compare_string(command, "grade-homework"))
    {
        int show_command_usage = 0;
        int should_match_title = 0;
        for(char *option = next_option(parser); option; option = next_option(parser))
        {
            if(compare_string(option, "--help")) { show_command_usage = 1; }
            else if(compare_string(option, "--match-title")) { should_match_title = 1; }
            else
            {
                show_command_usage = 1;
                write_error("unknown option '%s'", option);
            }
        }

        char *title = next_arg(parser);
        char *template_repo = next_arg(parser);
        char *template_branch = next_arg(parser);
        char *in_time = next_arg(parser);
        char *cutoff_time = next_arg(parser);
        char *out_path = next_arg(parser);
        if(!title || !template_repo || !in_time || !cutoff_time || !out_path || show_command_usage)
        {
            char *usage =
                "usage: hand2 grade-homework [--command-option] ... title template_repo template_branch deadline cutoff_time out_path"  "\n"
                                                                                                                                        "\n"
                "[arguments]"                                                                                                           "\n"
                "    title              title of the homework"                                                                          "\n"
                "    template_repo      homework template repository name"                                                              "\n"
                "    template_branch    the template_repo branch that contains all testcases"                                           "\n"
                "    deadline           deadline of the homework in YYYY-MM-DD-hh-mm-ss"                                                "\n"
                "    cutoff_time        max late submission time after deadline (in days)"                                              "\n"
                "    out_path           output file that lists score for all students (in the same order of the"                        "\n"
                "                       spreadsheet)"                                                                                   "\n"
                "[command-options]"                                                                                                     "\n"
                "    --match-title      only grade for repository named 'title', instead of using `title` as a prefix"                  "\n"
                "                       to find repositories to grade"                                                                  "\n";
            write_output(usage);
            return;
        }
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        char *google_token = config->value[Config_google_token];
        char *spreadsheet_id = config->value[Config_spreadsheet];
        char *student_key = config->value[Config_key_username];
        char *id_key = config->value[Config_key_student_id];
        char *grade_command = config->value[Config_grade_command];
        char *feedback_repo = config->value[Config_feedback_repo];
        char *score_relative_path = config->value[Config_score_relative_path];
        int thread_count = atoi(config->value[Config_grade_thread_count]);
        time_t deadline = parse_time(in_time, TIME_ZONE_UTC8);
        time_t cutoff = deadline + atoi(cutoff_time) * 86400;
        grade_homework(title, out_path, deadline, cutoff, template_repo, template_branch, feedback_repo,
                       grade_command, score_relative_path, thread_count, should_match_title,
                       github_token, organization, google_token, spreadsheet_id, student_key, id_key);
    }
    else if(compare_string(command, "announce-grade"))
    {
        int show_command_usage = 0;
        for(char *option = next_option(parser); option; option = next_option(parser))
        {
            if(compare_string(option, "--help")) { show_command_usage = 1; }
            else
            {
                write_error("unknown option '%s'", option);
            }
        }

        char *title = next_arg(parser);
        if(!title || show_command_usage)
        {
            char *usage =
                "usage: hand2 announce-grade [--command-option] ... title"  "\n"
                                                                            "\n"
                "[arguments]"                                               "\n"
                "    title    title of the homework"                        "\n"
                "[command-options]"                                         "\n";
            write_output(usage);
            return;
        }
        char *feedback_repo = config->value[Config_feedback_repo];
        char *spreadsheet_id = config->value[Config_spreadsheet];
        char *student_key = config->value[Config_key_username];
        char *id_key = config->value[Config_key_student_id];
        char *google_token = config->value[Config_google_token];
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        announce_grade(title, feedback_repo, github_token, organization, google_token, spreadsheet_id, student_key, id_key);
    }
    else
    {
        write_error("unknown command %s", command);
    }

    if(g_log_file) { fclose(g_log_file); }
}

static void
run_hand(int arg_count, char **args)
{
    static char config_path[MAX_PATH_LEN];
    if(!format_string(config_path, MAX_PATH_LEN, "%s/config.txt", g_root_dir))
    {
        return;
    }

    if(!ensure_config_exists(config_path))
    {
        write_error("config not found, default config created at '%s'", config_path);
        return;
    }

    Config config;
    if(load_config(&config, config_path))
    {
        ArgParser parser;
        init_arg_parser(&parser, args, arg_count);
        eval(&parser, &config);
    }
    else
    {
        write_error("unable to load config '%s'", config_path);
    }
}
