#include "hand.h"
#include "hand_buffer.c"
#include "hand_config.c"
#include "hand_curl.c"
#include "hand_git2.c"

static void
delete_cache_and_log(void)
{
    write_log("cache_dir=%s, log_dir=%s", g_cache_dir, g_log_dir);
    write_output("Deleting cache from '%s'...", g_cache_dir);
    platform.delete_directory(g_cache_dir);
    write_output("Deleting log from '%s'...", g_log_dir);
    platform.delete_directory(g_log_dir);
}

static void
config_check(Config *config)
{
    char *github_token = config->value[Config_github_token];
    char *google_token = config->value[Config_google_token];
    char *organization = config->value[Config_organization];
    char *ta_team = config->value[Config_ta_team];
    char *student_team = config->value[Config_student_team];
    char *grade_command = config->value[Config_grade_command];
    char *feedback_repo = config->value[Config_feedback_repo];
    char *spreadsheet_id = config->value[Config_spreadsheet];
    char *username_label = config->value[Config_username_label];
    char *id_label = config->value[Config_student_id_label];
    char *email = config->value[Config_email];
    char *grade_thread_count = config->value[Config_grade_thread_count];
    char *penalty_per_day = config->value[Config_penalty_per_day];
    char *score_relative_path = config->value[Config_score_relative_path];
    write_log("org=%s, ta_team=%s, student_team=%s, grade_command=%s, feedback_repo=%s, "
              "spreadsheet_id=%s, username_label=%s, id_label=%s, email=%s, thread_count=%s, penalty=%s, score_path=%s",
              organization, ta_team, student_team, grade_command, feedback_repo,
              spreadsheet_id, username_label, id_label, email, grade_thread_count, penalty_per_day, score_relative_path);

    int penalty_is_int = 1;
    for(char *c = penalty_per_day; *c; ++c)
    {
        if(*c < '0' && '9' < *c)
        {
            penalty_is_int = 0;
            break;
        }
    }

    static char username[MAX_URL_LEN];
    StringArray spreadsheet = {0};
    int ok[Config_one_past_last];
    for(int i = 0; i < Config_one_past_last; ++i)
        ok[i] = 1;

    ok[Config_github_token] = retrieve_username(username, MAX_URL_LEN, github_token);
    if(ok[Config_github_token])
    {
        ok[Config_organization] = github_organization_exists(github_token, organization);
        ok[Config_ta_team] = github_team_exists(github_token, organization, ta_team);
        ok[Config_student_team] = github_team_exists(github_token, organization, student_team);
        ok[Config_feedback_repo] = github_repository_exists(github_token, organization, feedback_repo);
    }
    ok[Config_google_token] = google_token_is_valid(google_token);
    if(ok[Config_google_token])
    {
        ok[Config_spreadsheet] = retrieve_spreadsheet(&spreadsheet, google_token, spreadsheet_id);
    }
    ok[Config_grade_thread_count] = (atoi(grade_thread_count) > 0);
    ok[Config_penalty_per_day] = penalty_is_int;

    write_output("");
    write_output("[GitHub]");
    write_output("    Token owner: %s ... %s", username, ok[Config_github_token] ? "OK" : "Not found");
    if(ok[Config_github_token])
    {
        write_output("    Organization: %s ... %s", organization, ok[Config_organization] ? "OK" : "Not found");
        write_output("    Teaching team: %s ... %s", ta_team, ok[Config_ta_team] ? "OK" : "Not found");
        write_output("    Student team: %s ... %s", student_team, ok[Config_student_team] ? "OK" : "Not found");
        write_output("    Feedback repository: %s ... %s", feedback_repo, ok[Config_feedback_repo] ? "OK" : "Not found");
        write_output("    Commit email: %s ... nocheck", email);
    }

    write_output("");
    write_output("[Google Sheet]");
    write_output("    Token ... %s", ok[Config_google_token] ? "OK" : "Not found");
    if(ok[Config_google_token])
    {
        write_output("    Spreadsheet: %s ... %s", spreadsheet_id, ok[Config_spreadsheet] ? "OK" : "Not found");
        if(ok[Config_spreadsheet])
        {
            write_output("");
            write_output("[Homework sheet]");
            for(int i = 0; i < spreadsheet.count; ++i)
            {
                static char buffer[4096];
                Sheet sheet = retrieve_sheet(google_token, spreadsheet_id, spreadsheet.elem[i]);
                int username_x = find_label(&sheet, username_label);
                int id_x = find_label(&sheet, id_label);
                format_string(buffer, sizeof(buffer), "    %s: %s=%d, %s=%d ... %s",
                              spreadsheet.elem[i],
                              username_label, (username_x != -1) ? username_x + 1 : -1,
                              id_label, (id_x != -1) ? id_x + 1 : -1,
                              (username_x != -1 && id_x != -1) ? "OK" : "Not a homework");
                write_output(buffer);
                free_sheet(&sheet);
            }
        }
    }

    write_output("");
    write_output("[Misc]");
    write_output("    Use %s grading thread ... %s", grade_thread_count, ok[Config_grade_thread_count] ? "OK" : "Not a positive integer");
    write_output("    Penalty per day is %s%% ... %s", penalty_per_day, ok[Config_penalty_per_day] ? "OK" : "Not a integer");
    write_output("    Score read from: {hwdir}/%s ... nocheck", score_relative_path);
    write_output("    Grading command: %s ... nocheck", grade_command);

    write_output("");
    write_output("[Summary]");
    if(true_for_all(ok, Config_one_past_last))
    {
        write_output("    Config is OK");
    }
    else
    {
        for(int i = 0; i < Config_one_past_last; ++i)
        {
            if(!ok[i])
                write_output("    Invalid config: %s", g_config_name[i]);
        }
    }
    free_string_array(&spreadsheet);
}

static void
invite_students(char *path, char *github_token, char *organization, char *team)
{
    write_log("path=%s, org=%s, team=%s", path, organization, team);

    StringArray students = read_string_array_file(path);
    if(students.count > 0)
    {
        write_output("Retrieving existing members...");
        StringArray existing_students = retrieve_team_members(github_token, organization, team);
        write_output("Retrieving existing invitations...");
        StringArray existing_invitations = retrieve_existing_invitations(github_token, organization, team);
        write_output("Checking for valid usernames...");
        int *exist = (int *)allocate_memory(students.count * sizeof(*exist));
        github_users_exist(exist, &students, github_token);

        int invalid_count = 0;
        int existing_count = 0;
        int inviting_count = 0;
        int success_count = 0;
        int failure_count = 0;
        for(int i = 0; i < students.count; ++i)
        {
            for(int j = 0; j < existing_students.count; ++j)
            {
                if(compare_case_insensitive(students.elem[i], existing_students.elem[j]))
                {
                    ++existing_count;
                    students.elem[i] = 0;
                    break;
                }
            }

            if(!students.elem[i])
                continue;
            for(int j = 0; j < existing_invitations.count; ++j)
            {
                if(compare_case_insensitive(students.elem[i], existing_invitations.elem[j]))
                {
                    ++inviting_count;
                    students.elem[i] = 0;
                    break;
                }
            }

            if(!students.elem[i])
                continue;
            if(!exist[i])
            {
                write_error("GitHub user '%s' does not exist", students.elem[i]);
                ++invalid_count;
                students.elem[i] = 0;
                break;
            }
        }

        if(invalid_count == 0)
        {
            write_output("Inviting new members...");
            for(int i = 0; i < students.count; ++i)
            {
                if(!students.elem[i])
                    continue;
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
            write_output("    Existing members: %d", existing_count);
            write_output("    Existing invitations: %d", inviting_count);
        }
        else
        {
            write_output("ABORT: some usernames are invalid");
        }
        free_memory(exist);
        free_string_array(&existing_invitations);
        free_string_array(&existing_students);
    }
    else
    {
        write_error("Input file '%s' is empty", path);
    }
    free_string_array(&students);
}

static void
collect_homework(char *title, char *out_path, time_t deadline, time_t cutoff,
                 int penalty_per_day, int is_weekends_one_day, char *only_repo,
                 char *github_token, char *organization, char *google_token, char *spreadsheet_id, char *student_label)
{
    tm t0 = calendar_time(deadline);
    tm t1 = calendar_time(cutoff);
    write_log("title=%s, out_path=%s, org=%s, penalty=%d, is_weekends_one_day=%d, only_repo=%s, spreadsheet=%s, student_label=%s, "
              "deadline=%d-%02d-%02d_%02d:%02d:%02d, cutoff=%d-%02d-%02d_%02d:%02d:%02d",
              title, out_path, organization, penalty_per_day, is_weekends_one_day, only_repo, spreadsheet_id, student_label,
              t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec,
              t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

    FILE *out_file = platform.fopen(out_path, "wb");
    if(!out_file)
    {
        write_error("Unable to create '%s'", out_path);
        return;
    }

    StringArray repos = {0};
    if(only_repo)
    {
        repos = allocate_string_array();
        append_string_array(&repos, only_repo);
    }
    else
    {
        write_output("Retrieving repositories with prefix '%s'...", title);
        repos = retrieve_repos_by_prefix(github_token, organization, title);
    }

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
    int student_x = find_label(&sheet, student_label);
    for(int y = 0; y < sheet.height; ++y)
    {
        static char req_repo[256];
        char *student = get_value(&sheet, student_x, y);
        format_string(req_repo, sizeof(req_repo), "%s-%s", title, student);

        int delay = 0;
        int index = find_index_case_insensitive(&repos, req_repo);
        if(index != -1)
        {
            ++submission_count;
            if(push_time[index] > deadline)
            {
                delay = (int)((push_time[index] - deadline + 86399) / 86400);
                assert(delay > 0);
                if(is_weekends_one_day)
                {
                    int weekend_before_deadline = (start_weekday + 6) / 7 + start_weekday / 7;
                    int weekend_before_cutoff = (start_weekday + delay + 6) / 7 + (start_weekday + delay) / 7;
                    int weekend_day_count = weekend_before_cutoff - weekend_before_deadline;
                    delay -= (weekend_day_count / 2);
                }

                if(late_submission_count == 0)
                {
                    write_output("");
                    write_output("[Late submission]");
                    write_output("%3s  %12s  %-19s  %-24s  %-40s",
                                 "#", "delay (days)", "push_time", "repository", "hash");
                }
                ++late_submission_count;
                tm t = calendar_time(push_time[index]);
                write_output("%3d  %12d  %4d-%02d-%02d_%02d:%02d:%02d  %-24s  %-40s",
                             late_submission_count, delay,
                             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                             repos.elem[index], hash[index].trim);
            }
        }

        if(!only_repo || index != -1)
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
    free_sheet(&sheet);
    free_memory(push_time);
    free_memory(hash);
    free_string_array(&branches);
    free_string_array(&repos);
}

static GrowableBuffer
format_report_by_file_replacement(GrowableBuffer *format, char *dir)
{
    GrowableBuffer result = allocate_growable_buffer();

    int depth = 0;
    char *identifier = 0;
    for(size_t i = 0; i < format->used; ++i)
    {
        char *c = format->memory + i;
        if(c[0] == '$' && c[1] == '$')
        {
            write_constant_string(&result, "$$");
            ++i;
        }
        else if(c[0] == '$' && c[1] == '{')
        {
            if(depth++ == 0)
                identifier = c + 2;

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
                    format_string(path, MAX_PATH_LEN, "%s/%s", dir, identifier);

                    GrowableBuffer replacement = read_entire_file(path);
                    write_growable_buffer(&result, replacement.memory, replacement.used);
                    free_growable_buffer(&replacement);
                }
                else
                {
                    write_constant_string(&result, "${");
                    write_growable_buffer(&result, identifier, string_len(identifier));
                    write_constant_string(&result, "}");
                }
                *c = '}';
            }
        }
        else if(depth == 0)
        {
            write_growable_buffer(&result, c, 1);
        }
    }
    return result;
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
grade_homework(char *title, char *out_path,
               time_t deadline, time_t cutoff, char *template_repo, char *template_branch, char *feedback_repo,
               char *command, char *score_relative_path, int dry, int thread_count, char *only_repo,
               char *github_token, char *organization, char *email,
               char *google_token, char *spreadsheet_id, char *student_label, char *id_label)
{
    tm t0 = calendar_time(deadline);
    tm t1 = calendar_time(cutoff);
    write_log("title=%s, out_path=%s, template_repo=%s, template_branch=%s, feedback_repo=%s, command=%s, score_path=%s, "
              "dry=%d, thread=%d, only_repo=%s, org=%s, email=%s, spreadsheet=%s, student_label=%s, id_label=%s, "
              "deadline=%d-%02d-%02d_%02d:%02d:%02d, cutoff=%d-%02d-%02d_%02d:%02d:%02d",
              title, out_path, template_repo, template_branch, feedback_repo, command, score_relative_path,
              dry, thread_count, only_repo, organization, email, spreadsheet_id, student_label, id_label,
              t0.tm_year + 1900, t0.tm_mon + 1, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec,
              t1.tm_year + 1900, t1.tm_mon + 1, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

    static char username[MAX_URL_LEN];
    static char template_dir[MAX_PATH_LEN];
    static char feedback_dir[MAX_PATH_LEN];
    static char template_test_dir[MAX_PATH_LEN];
    static char template_docker_dir[MAX_PATH_LEN];
    static char feedback_report_dir[MAX_PATH_LEN];
    static char feedback_homework_dir[MAX_PATH_LEN];
    static char report_template_path[MAX_PATH_LEN];

    if(!retrieve_username(username, MAX_URL_LEN, github_token))
    {
        write_error("Unable to retrieve username");
        return;
    }

    write_output("Retrieving homework template...");
    GitCommitHash template_hash = retrieve_latest_commit(github_token, organization, template_repo, template_branch);
    if(!cache_repository(template_dir, MAX_PATH_LEN, username, github_token, organization, template_repo, &template_hash))
    {
        write_error("Unable to retrieve '%s/%s'", organization, template_repo);
        return;
    }

    write_output("Retrieving feedback repository...");
    if(!cache_repository(feedback_dir, MAX_PATH_LEN, username, github_token, organization, feedback_repo, 0))
    {
        write_error("Unable to retrieve '%s/%s'", organization, feedback_repo);
        return;
    }

    format_string(template_test_dir, MAX_PATH_LEN, "%s/test", template_dir);
    format_string(template_docker_dir, MAX_PATH_LEN, "%s/docker", template_dir);
    format_string(feedback_homework_dir, MAX_PATH_LEN, "%s/%s",feedback_dir, title);
    format_string(feedback_report_dir, MAX_PATH_LEN, "%s/%s/reports",feedback_dir, title);
    format_string(report_template_path, MAX_PATH_LEN, "%s/report_template_%s.md", feedback_dir, title);

    FILE *out_file = platform.fopen(out_path, "wb");
    if(!out_file)
    {
        write_error("Unable to create '%s'", out_path);
        return;
    }

    platform.create_directory(feedback_homework_dir);
    platform.create_directory(feedback_report_dir);
    GrowableBuffer report_template = read_entire_file(report_template_path);

    StringArray repos = {0};
    if(only_repo)
    {
        repos = allocate_string_array();
        append_string_array(&repos, only_repo);
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
        if(cache_repository(dir, MAX_PATH_LEN, username, github_token, organization, repos.elem[i], &hash[i]))
        {
            format_string(test_dir, MAX_PATH_LEN, "%s/test", dir);
            format_string(docker_dir, MAX_PATH_LEN, "%s/docker", dir);
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
    }

    int submission_count = 0;
    int late_submission_count = 0;
    int failure_count = 0;
    write_output("Start grading...");
    platform.wait_for_completion(thread_count, repos.count, works, grade_homework_on_progress, grade_homework_on_complete);

    write_output("Generating report...");
    Sheet sheet = retrieve_sheet(google_token, spreadsheet_id, title);
    int student_x = find_label(&sheet, student_label);
    int id_x = find_label(&sheet, id_label);
    for(int y = 0; y < sheet.height; ++y)
    {
        static char req_name[256];
        char *student = get_value(&sheet, student_x, y);
        char *student_id = get_value(&sheet, id_x, y);
        format_string(req_name, sizeof(req_name), "%s-%s", title, student);

        double score = 0;
        int index = find_index_case_insensitive(&repos, req_name);
        if(index != -1)
        {
            ++submission_count;
            if(push_time[index] > deadline)
                ++late_submission_count;
            if(works[index].exit_code != 0)
                ++failure_count;

            if(works[index].exit_code == 0)
            {
                static char report_path[MAX_PATH_LEN];
                static char score_path[MAX_PATH_LEN];
                format_string(report_path, MAX_PATH_LEN, "%s/%s.md", feedback_report_dir, student_id);
                format_string(score_path, MAX_PATH_LEN, "%s/%s", works[index].work_dir, score_relative_path);

                platform.delete_file(report_path);
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

                GrowableBuffer report = format_report_by_file_replacement(&report_template, works[index].work_dir);
                FILE *report_file = platform.fopen(report_path, "wb");
                if(report_file)
                {
                    fwrite(report.memory, report.used, 1, report_file);
                    fclose(report_file);
                }
                free_growable_buffer(&report);
            }
        }

        if(!only_repo || index != -1)
            fprintf(out_file, "%f\n", score);
    }

    if(dry)
    {
        write_output("DRY RUN: reports are not pushed, you can view the generated reports at '%s'", feedback_report_dir);
    }
    else
    {
        write_output("Pushing reports...");
        static char commit_message[64];
        format_string(commit_message, sizeof(commit_message), "update %s report", title);
        if(!push_to_remote(feedback_dir, github_token, commit_message, username, email))
        {
            write_error("Unable to push the reports");
        }
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
    free_sheet(&sheet);
    free_memory(works);
    free_memory(hash);
    free_memory(push_time);
    free_string_array(&branches);
    free_string_array(&repos);
    free_growable_buffer(&report_template);
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
        char *label = 0;
        for(char *c = format->elem[y]; *c; ++c)
        {
            if(c[0] == '$' && c[1] == '$')
            {
                write_constant_string(&buffer, "$");
                ++c;
            }
            else if(c[0] == '$' && c[1] == '{')
            {
                if(depth++ == 0)
                    label = c + 2;

                ++c;
            }
            else if(c[0] == '}')
            {
                if(depth == 1)
                {
                    *c = 0;
                    int x = find_label(sheet, label);
                    *c = '}';
                    if(x >= 0)
                    {
                        char *replacement = get_value(sheet, x, y);
                        write_growable_buffer(&buffer, replacement, string_len(replacement));
                    }
                    else
                    {
                        success = 0;
                        // NOTE: we assume every report has the same labels, only reports label not found for
                        // the first report
                        if(y == 0)
                            write_error("Label not found: %s", label);
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
        free_string_array(out);

    return success;
}

static void
announce_grade(char *title, char *feedback_repo, int dry, char *only_repo,
               char *github_token, char *organization, char *google_token, char *spreadsheet_id, char *student_label, char *id_label)
{
    write_log("title=%s, feedback_repo=%s, org=%s, dry=%d, only_repo=%s, spreadsheet=%s, student_label=%s, id_label=%s",
              title, feedback_repo, organization, dry, only_repo, spreadsheet_id, student_label, id_label);

    static char issue_title[256];
    static char username[MAX_URL_LEN];
    static char feedback_dir[MAX_PATH_LEN];
    static char feedback_report_dir[MAX_PATH_LEN];

    if(!retrieve_username(username, MAX_URL_LEN, github_token))
    {
        write_error("Unable to retrieve username");
        return;
    }

    write_output("Retrieving feedback repository...");
    if(!cache_repository(feedback_dir, MAX_PATH_LEN, username, github_token, organization, feedback_repo, 0))
    {
        write_error("Unable to retrieve '%s/%s'", organization, feedback_repo);
        return;
    }

    format_string(issue_title, sizeof(issue_title), "Grade for %s", title);
    format_string(feedback_report_dir, MAX_PATH_LEN, "%s/%s/reports", feedback_dir, title);

    StringArray repos = {0};
    if(only_repo)
    {
        repos = allocate_string_array();
        append_string_array(&repos, only_repo);
    }
    else
    {
        write_output("Retrieving repos with prefix '%s'...", title);
        repos = retrieve_repos_by_prefix(github_token, organization, title);
    }

    write_output("Retrieving issue '%s'...", issue_title);
    int *issue_numbers = (int *)allocate_memory(repos.count * sizeof(*issue_numbers));
    retrieve_issue_numbers_by_title(issue_numbers, &repos, github_token, organization, issue_title);

    int announcement_count = 0;
    int failure_count = 0;
    Sheet sheet = retrieve_sheet(google_token, spreadsheet_id, title);
    if(sheet.width && sheet.height)
    {
        int student_x = find_label(&sheet, student_label);
        int id_x = find_label(&sheet, id_label);
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
                free_growable_buffer(&content);
            }

            StringArray issue_bodies;
            if(format_feedback_issues(&issue_bodies, &reports, &sheet))
            {
                for(int y = 0; y < sheet.height; ++y)
                {
                    static char req_repo[256];
                    char *student = get_value(&sheet, student_x, y);
                    format_string(req_repo, sizeof(req_repo), "%s-%s", title, student);

                    int index = find_index_case_insensitive(&repos, req_repo);
                    if(index == -1) continue;

                    write_output("Announce grade for '%s'", repos.elem[index]);
                    ++announcement_count;
                    if(issue_numbers[index])
                    {
                        if(dry)
                        {
                            write_output("DRY RUN: attempt to edit issue '%s #%d' to:", issue_title, issue_numbers[index]);
                            write_output("%s", issue_bodies.elem[y]);
                            write_output("----------------------------------------------------------------");
                        }
                        else if(!edit_issue(github_token, organization, repos.elem[index], issue_title, issue_bodies.elem[y], issue_numbers[index]))
                        {
                            ++failure_count;
                        }
                    }
                    else
                    {
                        if(dry)
                        {
                            write_output("DRY RUN: attempt to create issue '%s #%d' with content:", issue_title, issue_numbers[index]);
                            write_output("%s", issue_bodies.elem[y]);
                            write_output("----------------------------------------------------------------");
                        }
                        else if(!create_issue(github_token, organization, repos.elem[index], issue_title, issue_bodies.elem[y]))
                        {
                            ++failure_count;
                        }
                    }
                    // NOTE: sleep to bypass the github rate limit
                    // TODO: need further investigation on the error messages github returns
                    platform.sleep(1000);
                }
                free_string_array(&issue_bodies);
            }
            free_string_array(&reports);

            write_output("");
            write_output("[Summary]");
            write_output("    Total students: %d", sheet.height);
            write_output("    Total announcement: %d", announcement_count);
            write_output("    Failed announcement: %d", failure_count);
        }
        else
        {
            if(student_x < 0)
                write_error("Label '%s' not found", student_label);
            if(id_x < 0)
                write_error("Label '%s' not found", id_label);
        }
    }
    else
    {
        write_error("Sheet '%s' is empty under spreadsheet id '%s'", title, spreadsheet_id);
    }
    free_sheet(&sheet);
    free_memory(issue_numbers);
    free_string_array(&repos);
}

static void
run_hand(int arg_count, char **args)
{
    static char log_path[MAX_PATH_LEN];
    static char config_path[MAX_PATH_LEN];
    struct tm time = current_calendar_time();
    format_string(config_path, MAX_PATH_LEN, "%s/config.txt", g_root_dir);
    format_string(log_path, MAX_PATH_LEN, "%s/%d-%02d-%02d_%02d-%02d-%02d.log",
                  g_log_dir, time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                  time.tm_hour, time.tm_min, time.tm_sec);

    // NOTE: parse options
    int show_usage = 0;
    ArgParser parser = init_arg_parser(arg_count, args);
    for(;;)
    {
        char *option = next_option(&parser);
        if(!option)
            break;

        if(compare_string(option, "--help"))
        {
            show_usage = 1;
        }
        else if(compare_string(option, "--log"))
        {
            platform.create_directory(g_log_dir);
            g_log_file = platform.fopen(log_path, "wb");
        }
        else
        {
            show_usage = 1;
            write_error("Unknown option '%s'", option);
        }
    }

    // NOTE: if config does not exist, show usage and quit
    if(!ensure_config_exists(config_path))
    {
        show_usage = 1;
        write_output("config not found, default config created at '%s'", config_path);
        write_output("");
    }

    char *command = next_arg(&parser);
    if(show_usage || ! command)
    {
        char *usage =
            "usage: hand2 [--options] ... command [--command-options] ... [args] ..."   "\n"
            "[options]"                                                                 "\n"
            "    --help    show this message"                                           "\n"
            "    --log     log to log/YYYY-MM-DD_hh-mm-ss.log"                          "\n"
            "[command]"                                                                 "\n"
            "    config-check       check if the config is valid"                       "\n"
            "    invite-students    invite student into github organization"            "\n"
            "    collect-homework   collect homework and retrieve late submission info" "\n"
            "    grade-homework     grade homework"                                     "\n"
            "    announce-grade     announce grade"                                     "\n"
            "    clean              delete unrelated files such as logs and caches"     "\n"
            "[common-command-options]"                                                  "\n"
            "    --help             show the help message regards to the command"       "\n";
        write_output(usage);
        goto CLEANUP;
    }

    Config config;
    if(!load_config(&config, config_path))
    {
        write_error("Unable to load config '%s'", config_path);
        goto CLEANUP;
    }

    // NOTE: parse command
    if(compare_string(command, "clean"))
    {
        int show_command_usage = 0;
        for(char *option = next_option(&parser); option; option = next_option(&parser))
        {
            if(compare_string(option, "--help"))
                show_command_usage = 1;
            else
            {
                show_command_usage = 1;
                write_error("Unknown option '%s'", option);
            }
        }

        if(next_arg(&parser))
        {
            show_command_usage = 1;
            write_error("Too many arguments");
        }

        if(show_command_usage)
        {
            char *usage =
                "usage: hand2 clean"    "\n"
                                        "\n"
                "[arguments]"           "\n"
                ""                      "\n"
                "[command-options]"     "\n";
            write_output(usage);
            goto CLEANUP;
        }
        delete_cache_and_log();
    }
    else if(compare_string(command, "config-check"))
    {
        int show_command_usage = 0;
        for(char *option = next_option(&parser); option; option = next_option(&parser))
        {
            if(compare_string(option, "--help"))
                show_command_usage = 1;
            else
            {
                show_command_usage = 1;
                write_error("Unknown option '%s'", option);
            }
        }

        if(next_arg(&parser))
        {
            show_command_usage = 1;
            write_error("Too many arguments");
        }

        if(show_command_usage)
        {
            char *usage =
                "usage: hand2 config-check" "\n"
                                            "\n"
                "[arguments]"               "\n"
                ""                          "\n"
                "[command-options]"         "\n";
            write_output(usage);
            goto CLEANUP;
        }
        config_check(&config);
    }
    else if(compare_string(command, "invite-students"))
    {
        int show_command_usage = 0;
        for(char *option = next_option(&parser); option; option = next_option(&parser))
        {
            if(compare_string(option, "--help"))
                show_command_usage = 1;
            else
            {
                show_command_usage = 1;
                write_error("Unknown option '%s'", option);
            }
        }

        char *input_path = next_arg(&parser);
        if(next_arg(&parser))
        {
            show_command_usage = 1;
            write_error("Too many arguments");
        }

        if(!input_path || show_command_usage)
        {
            char *usage =
                "usage: hand2 invite-students [--command-options] ... path" "\n"
                                                                            "\n"
                "[arguments]"                                               "\n"
                "    path    file path of listed github handle to invite"   "\n"
                "[command-options]"                                         "\n";
            write_output(usage);
            goto CLEANUP;
        }
        char *github_token = config.value[Config_github_token];
        char *organization = config.value[Config_organization];
        char *team = config.value[Config_student_team];
        invite_students(input_path, github_token, organization, team);
    }
    else if(compare_string(command, "collect-homework"))
    {
        int show_command_usage = 0;
        int is_weekends_one_day = 1;
        char *only_repo = 0;
        for(char *option = next_option(&parser); option; option = next_option(&parser))
        {
            if(compare_string(option, "--help"))
                show_command_usage = 1;
            else if(compare_string(option, "--only-repo"))
                only_repo = next_arg(&parser);
            else if(compare_string(option, "--no-weekends"))
                is_weekends_one_day = 0;
            else
            {
                show_command_usage = 1;
                write_error("Unknown option '%s'", option);
            }
        }

        char *title = next_arg(&parser);
        char *in_time = next_arg(&parser);
        char *cutoff_time = next_arg(&parser);
        char *out_path = next_arg(&parser);
        if(next_arg(&parser))
        {
            show_command_usage = 1;
            write_error("Too many arguments");
        }

        if(!title || !in_time || !cutoff_time || show_command_usage)
        {
            char *usage =
                "usage: hand2 collect-homework [--command-option] ... title deadline cutoff_time out_path"          "\n"
                                                                                                                    "\n"
                "[arguments]"                                                                                       "\n"
                "    title                 title of the homework"                                                   "\n"
                "    deadline              deadline of the homework in YYYY-MM-DD-hh-mm-ss"                         "\n"
                "    cutoff_time           max late submission time after deadline (in days)"                       "\n"
                "    out_path              output file that lists penalty for all students (in the same order of"   "\n"
                "                          the spreadsheet)"                                                        "\n"
                "[command-options]"                                                                                 "\n"
                "    --only-repo <name>    only collect homework for the repository named <name>"                   "\n"
                "    --no-weekends         weekends are not considered as one day"                                  "\n";
            write_output(usage);
            goto CLEANUP;
        }
        char *github_token = config.value[Config_github_token];
        char *organization = config.value[Config_organization];
        char *google_token = config.value[Config_google_token];
        char *spreadsheet_id = config.value[Config_spreadsheet];
        char *student_label = config.value[Config_username_label];
        int penalty_per_day = atoi(config.value[Config_penalty_per_day]);
        time_t deadline = parse_time(in_time, TIME_ZONE_UTC8);
        time_t cutoff = deadline + atoi(cutoff_time) * 86400;
        if(!deadline)
        {
            write_error("Invalid time format: %s, requires YYYY-MM-DD-hh-mm-ss", in_time);
            goto CLEANUP;
        }
        collect_homework(title, out_path, deadline, cutoff, penalty_per_day, is_weekends_one_day, only_repo,
                         github_token, organization, google_token, spreadsheet_id, student_label);
    }
    else if(compare_string(command, "grade-homework"))
    {
        int show_command_usage = 0;
        int dry = 0;
        char *only_repo = 0;
        for(char *option = next_option(&parser); option; option = next_option(&parser))
        {
            if(compare_string(option, "--help"))
                show_command_usage = 1;
            else if(compare_string(option, "--dry"))
                dry = 1;
            else if(compare_string(option, "--only-repo"))
                only_repo = next_arg(&parser);
            else
            {
                show_command_usage = 1;
                write_error("Unknown option '%s'", option);
            }
        }

        char *title = next_arg(&parser);
        char *template_repo = next_arg(&parser);
        char *template_branch = next_arg(&parser);
        char *in_time = next_arg(&parser);
        char *cutoff_time = next_arg(&parser);
        char *out_path = next_arg(&parser);
        if(next_arg(&parser))
        {
            show_command_usage = 1;
            write_error("Too many arguments");
        }

        if(!title || !template_repo || !in_time || !cutoff_time || !out_path || show_command_usage)
        {
            char *usage =
                "usage: hand2 grade-homework [--command-option] ... title template_repository template_branch deadline cutoff_day out_path" "\n"
                                                                                                                                            "\n"
                "[arguments]"                                                                                                               "\n"
                "    title                 title of the homework"                                                                           "\n"
                "    template_repo         homework template repository name"                                                               "\n"
                "    template_branch       the template_repo branch that contains all testcases"                                            "\n"
                "    deadline              deadline of the homework in YYYY-MM-DD-hh-mm-ss"                                                 "\n"
                "    cutoff_time           max late submission time after deadline (in days)"                                               "\n"
                "    out_path              output file that lists score for all students (in the same order of the"                         "\n"
                "                          spreadsheet)"                                                                                    "\n"
                "[command-options]"                                                                                                         "\n"
                "    --dry                 perform a trial run without making any remote changes"                                           "\n"
                "    --only-repo <name>    only grade homework for the repository named <name>"                                             "\n";
            write_output(usage);
            goto CLEANUP;
        }
        char *github_token = config.value[Config_github_token];
        char *organization = config.value[Config_organization];
        char *email = config.value[Config_email];
        char *google_token = config.value[Config_google_token];
        char *spreadsheet_id = config.value[Config_spreadsheet];
        char *student_label = config.value[Config_username_label];
        char *id_label = config.value[Config_student_id_label];
        char *grade_command = config.value[Config_grade_command];
        char *feedback_repo = config.value[Config_feedback_repo];
        char *score_relative_path = config.value[Config_score_relative_path];
        int thread_count = atoi(config.value[Config_grade_thread_count]);
        time_t deadline = parse_time(in_time, TIME_ZONE_UTC8);
        time_t cutoff = deadline + atoi(cutoff_time) * 86400;
        if(!deadline)
        {
            write_error("Invalid time format: %s, requires YYYY-MM-DD-hh-mm-ss", in_time);
            goto CLEANUP;
        }
        grade_homework(title, out_path, deadline, cutoff, template_repo, template_branch, feedback_repo,
                       grade_command, score_relative_path, dry, thread_count, only_repo,
                       github_token, organization, email, google_token, spreadsheet_id, student_label, id_label);
    }
    else if(compare_string(command, "announce-grade"))
    {
        int show_command_usage = 0;
        int dry = 0;
        char *only_repo = 0;
        for(char *option = next_option(&parser); option; option = next_option(&parser))
        {
            if(compare_string(option, "--help"))
                show_command_usage = 1;
            else if(compare_string(option, "--dry"))
                dry = 1;
            else if(compare_string(option, "--only-repo"))
                only_repo = next_arg(&parser);
            else
                write_error("Unknown option '%s'", option);
        }

        char *title = next_arg(&parser);
        if(next_arg(&parser))
        {
            show_command_usage = 1;
            write_error("Too many arguments");
        }

        if(!title || show_command_usage)
        {
            char *usage =
                "usage: hand2 announce-grade [--command-option] ... title"                          "\n"
                                                                                                    "\n"
                "[arguments]"                                                                       "\n"
                "    title    title of the homework"                                                "\n"
                "[command-options]"                                                                 "\n"
                "    --dry                 perform a trial run without making any remote changes"   "\n"
                "    --only-repo <name>    only announce grade for the repository named <name>"     "\n";
            write_output(usage);
            goto CLEANUP;
        }
        char *feedback_repo = config.value[Config_feedback_repo];
        char *spreadsheet_id = config.value[Config_spreadsheet];
        char *student_label = config.value[Config_username_label];
        char *id_label = config.value[Config_student_id_label];
        char *google_token = config.value[Config_google_token];
        char *github_token = config.value[Config_github_token];
        char *organization = config.value[Config_organization];
        announce_grade(title, feedback_repo, dry, only_repo, github_token, organization,
                       google_token, spreadsheet_id, student_label, id_label);
    }
    else
    {
        write_error("Unknown command %s", command);
    }

CLEANUP:
    free_config(&config);
    if(g_log_file)
        fclose(g_log_file);
}
