#include "hand.h"
#include "hand_buffer.c"
#include "hand_config.c"
#include "hand_curl.c"
#include "hand_git2.c"

static void
invite_students(char *input_path, char *github_token, char *organization, char *team)
{
    write_log("[invite students] path='%s', org='%s', team='%s'", input_path, organization, team);

    StringArray users_to_invite = read_string_array_file(input_path);
    if(users_to_invite.count > 0)
    {
        write_output("Retrieving existing members...");
        StringArray existing_users = retrieve_users_in_team(github_token, organization, team);
        write_output("Retrieving existing invitations...");
        StringArray existing_invitations = retrieve_existing_invitations(github_token, organization, team);

        write_log("existing member count: %d", existing_users.count);
        write_log("existing invitation count: %d", existing_invitations.count);

        int invalid_count = 0;
        for(int i = 0; i < users_to_invite.count; ++i)
        {
            for(int j = 0; j < existing_users.count; ++j)
            {
                if(compare_case_insensitive(users_to_invite.elem[i], existing_users.elem[j]))
                {
                    users_to_invite.elem[i] = 0;
                    break;
                }
            }

            if(!users_to_invite.elem[i]) continue;
            for(int j = 0; j < existing_invitations.count; ++j)
            {
                if(compare_case_insensitive(users_to_invite.elem[i], existing_invitations.elem[j]))
                {
                    users_to_invite.elem[i] = 0;
                    break;
                }
            }

            if(!users_to_invite.elem[i]) continue;
            if(!github_user_exists(github_token, users_to_invite.elem[i]))
            {
                write_error("github user '%s' does not exist", users_to_invite.elem[i]);
                ++invalid_count;
                users_to_invite.elem[i] = 0;
                break;
            }
        }

        if(invalid_count == 0)
        {
            int failure_count = 0;
            int success_count = 0;
            write_output("Inviting new members...");
            for(int i = 0; i < users_to_invite.count; ++i)
            {
                if(!users_to_invite.elem[i]) continue;
                if(invite_user_to_team(github_token, users_to_invite.elem[i], organization, team))
                {
                    ++success_count;
                    write_output("Invite '%s' to '%s/%s'", users_to_invite.elem[i], organization, team);
                }
                else
                {
                    ++failure_count;
                    write_error("Can't invite '%s' to '%s/%s'", users_to_invite.elem[i], organization, team);
                }
            }

            write_output("");
            write_output("[Summary]");
            write_output("    Total students: %d", users_to_invite.count);
            write_output("    New invitation (success): %d", success_count);
            write_output("    New invitation (failure): %d", failure_count);
            write_output("    Existing members: %d", existing_users.count);
            write_output("    Existing invitations: %d", existing_invitations.count);
        }
        else
        {
            write_output("ABORT: some usernames are invalid");
        }
        free_string_array(&existing_invitations);
        free_string_array(&existing_users);
    }
    else
    {
        write_error("input file '%s' is empty", input_path);
    }
    free_string_array(&users_to_invite);
}

static void
collect_homework(char *title, char *github_token, char *organization, time_t late_submission_start, time_t late_submission_end)
{
    tm t0 = calendar_time(late_submission_start);
    tm t1 = calendar_time(late_submission_end);
    write_log("[collect homework] title='%s', organization='%s', "
              "late_submission_start='%d-%02d-%02d_%02d:%02d:%02d', late_submission_end='%d-%02d-%02d_%02d:%02d:%02d'",
              title, organization,
              t0.tm_year, t0.tm_mon, t0.tm_mday, t0.tm_hour, t0.tm_min, t0.tm_sec,
              t1.tm_year, t1.tm_mon, t1.tm_mday, t1.tm_hour, t1.tm_min, t1.tm_sec);

    write_output("Retrieving repos with prefix '%s'...", title);
    StringArray repos = retrieve_repos_by_prefix(github_token, organization, title);

    write_output("Retrieving default branches...");
    StringArray default_branchs = retrieve_default_branchs(&repos, github_token, organization);
    assert(default_branchs.count == repos.count);

    // TODO: move into retrieve_pushes_before_time()
    write_output("Retrieving latest commits...");
    GitCommitHash *latest_commits = (GitCommitHash *)allocate_memory(repos.count * sizeof(*latest_commits));
    retrieve_latest_commits(latest_commits, &repos, &default_branchs, github_token, organization);

    write_output("Retrieving pushes before deadline...");
    GitCommitHash *hash = (GitCommitHash *)allocate_memory(repos.count * sizeof(*hash));
    time_t *push_time = (time_t *)allocate_memory(repos.count * sizeof(*push_time));
    retrieve_pushes_before_time(hash, push_time, &repos, &default_branchs, latest_commits,
                                github_token, organization, late_submission_end);

    write_output("[Late submission]");
    write_output("    %2s %-5s  %-24s  %-19s  %-40s", "#", "delay", "repo_name", "push_time", "hash");
    int late_submission_count = 0;
    for(int i = 0; i < repos.count; ++i)
    {
        if(push_time[i] > late_submission_start)
        {
            ++late_submission_count;
            int delay = (int)((push_time[i] - late_submission_start + 86400 - 1) / 86400);
            tm t = calendar_time(push_time[i]);
            write_output("    %2d %1d-day  %-24s  %d-%02d-%02d_%02d:%02d:%02d  %-8s",
                         late_submission_count, delay, repos.elem[i],
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                         hash[i].string);
        }
    }
    write_output("[Summary]");
    write_output("    Total submission: %d", repos.count);
    write_output("    Late submission: %d", late_submission_count);
    free_memory(push_time);
    free_memory(hash);
    free_memory(latest_commits);
    free_string_array(&default_branchs);
    free_string_array(&repos);
}

static void
grade_homework(char *title, char *template_repo_name, char *github_token, char *organization,
               time_t late_submission_start, time_t late_submission_end, char *command)
{
    (void)late_submission_start;
    write_output("retrieving repos with prefix '%s'...", title);
    StringArray repo_names = retrieve_repos_by_prefix(github_token, organization, title);

    time_t *push_time = (time_t *)allocate_memory(repo_names.count * sizeof(*push_time));
    GitCommitHash *hash = (GitCommitHash *)allocate_memory(repo_names.count * sizeof(*hash));
    GitCommitHash *latest_commits = (GitCommitHash *)allocate_memory(repo_names.count * sizeof(*latest_commits));
    write_output("retrieving default branches...");
    StringArray default_branchs = retrieve_default_branchs(&repo_names, github_token, organization);
    assert(default_branchs.count == repo_names.count);
    write_output("retrieving latest commits...");
    retrieve_latest_commits(latest_commits, &repo_names, &default_branchs, github_token, organization);
    write_output("retrieving pushes before deadline...");
    retrieve_pushes_before_time(hash, push_time, &repo_names, &default_branchs, latest_commits,
                                github_token, organization, late_submission_end);

    static char username[MAX_URL_LEN];
    static char source_url[MAX_URL_LEN];
    static char source_repo_dir[MAX_PATH_LEN];
    static char source_test_dir[MAX_PATH_LEN];
    static char source_docker_dir[MAX_PATH_LEN];
    if(retrieve_username(username, MAX_URL_LEN, github_token) &&
       format_string(source_url, MAX_URL_LEN,
                     "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, template_repo_name) &&
       format_string(source_repo_dir, MAX_PATH_LEN, "%s/cache/%s", global_root_dir, template_repo_name) &&
       format_string(source_test_dir, MAX_PATH_LEN, "%s/test", source_repo_dir) &&
       format_string(source_docker_dir, MAX_PATH_LEN, "%s/docker", source_repo_dir))
    {
        sync_repository(source_repo_dir, source_url, 0);
        for(int i = 0; i < repo_names.count; ++i)
        {
            static char target_url[MAX_URL_LEN];
            static char target_repo_dir[MAX_PATH_LEN];
            static char target_test_dir[MAX_PATH_LEN];
            static char target_docker_dir[MAX_PATH_LEN];
            if(format_string(target_url, MAX_URL_LEN,
                             "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, repo_names.elem[i]) &&
               format_string(target_repo_dir, MAX_PATH_LEN, "%s/cache/%s_%s", global_root_dir, repo_names.elem[i], hash[i].string) &&
               format_string(target_test_dir, MAX_PATH_LEN, "%s/test", target_repo_dir) &&
               format_string(target_docker_dir, MAX_PATH_LEN, "%s/docker", target_repo_dir))
            {
                sync_repository(target_repo_dir, target_url, &hash[i]);
                platform.delete_directory(target_test_dir);
                platform.delete_directory(target_docker_dir);
                platform.copy_directory(target_test_dir, source_test_dir);
                platform.copy_directory(target_docker_dir, source_docker_dir);
                platform.create_process(command, target_repo_dir);
            }
        }
    }
}

static StringArray
list_files_with_extension(char *dir, char *extension)
{
    GrowableBuffer buffer = allocate_growable_buffer();
    static char path[MAX_PATH_LEN];

    FileIter file_iter;
    for(file_iter = platform.begin_iterate_file(path, MAX_PATH_LEN, dir, extension);
        platform.file_iter_is_valid(&file_iter);
        platform.file_iter_advance(&file_iter))
    {
        write_growable_buffer(&buffer, path, string_len(path));
        write_constant_string(&buffer, "\0");
    }
    platform.end_iterate_file(&file_iter);
    return split_null_terminated_strings(&buffer);
}

static int
format_feedback_issues(StringArray *out, char *template, Sheet *sheet, StringArray *postfixes)
{
    int success = 1;
    assert(sheet->height == postfixes->count);

    GrowableBuffer buffer = allocate_growable_buffer();
    for(int y = 0; y < sheet->height; ++y)
    {
        int depth = 0;
        char *identifier = 0;
        for(char *c = template; *c; ++c)
        {
            if(c[0] == '\\' && (c[1] == '$' || c[1] == '{' || c[1] == '}'))
            {
                write_growable_buffer(&buffer, &c[1], 1);
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
                        char *substituded = get_value(sheet, x, y);
                        write_growable_buffer(&buffer, substituded, string_len(substituded));
                    }
                    else
                    {
                        // TODO: error
                        success = 0;
                    }
                }

                if(depth > 0) { --depth; }
                else
                {
                    // TODO: error
                }
            }
            else
            {
                write_growable_buffer(&buffer, c, 1);
            }
        }
        write_growable_buffer(&buffer, postfixes->elem[y], string_len(postfixes->elem[y]));
        write_growable_buffer(&buffer, "\0", 1);
    }


    if(success)
    {
        *out = split_null_terminated_strings(&buffer);
        assert(out->count == sheet->height);
    }
    else
    {
        clear_memory(out, sizeof(*out));
        free_growable_buffer(&buffer);
    }
    return success;
}

static GrowableBuffer
read_entire_file(char *path)
{
    GrowableBuffer buffer = allocate_growable_buffer();
    FILE *file = fopen(path, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = (size_t)ftell(file);
        fseek(file, 0, SEEK_SET);
        reserve_growable_buffer(&buffer, file_size);
        fwrite(buffer.memory + buffer.used, file_size, 1, file);
        fclose(file);
    }
    return buffer;
}

static void
announce_grade(char *title, char *feedback_repo,
               char *google_token, char *spreadsheet_id, char *user_key, char *github_token, char *organization)
{
    write_log("[announce grade] title='%s', feedback_repo='%s', spreadsheet_id='%s', user_key='%s', organization='%s'",
              title, feedback_repo, spreadsheet_id, user_key, organization);

    Sheet grade_sheet = retrieve_sheet(google_token, spreadsheet_id, title);
    if(grade_sheet.width == 0 || grade_sheet.height == 0)
    {
        int user_x = find_key_index(&grade_sheet, user_key);
        if(user_x >= 0)
        {
            int student_count = grade_sheet.height;
            static char issue_title[256];
            static char username[MAX_URL_LEN];
            static char feedback_repo_url[MAX_URL_LEN];
            static char feedback_repo_dir[MAX_PATH_LEN];
            static char feedback_dir[MAX_PATH_LEN];
            static char template_path[MAX_PATH_LEN];
            if(retrieve_username(username, MAX_URL_LEN, github_token) &&
               format_string(issue_title, sizeof(issue_title), "Grade for %", title) &&
               format_string(feedback_repo_url, MAX_URL_LEN,
                             "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, feedback_repo) &&
               format_string(feedback_repo_dir, MAX_PATH_LEN, "%s/cache/feedback", global_root_dir) &&
               format_string(feedback_dir, MAX_PATH_LEN, "%s/%s", feedback_repo_dir, title) &&
               format_string(template_path, MAX_PATH_LEN, "%s/template.md", feedback_repo_dir))
            {
                // NOTE: repos_buffer will be free by repos
                GrowableBuffer repos_buffer = allocate_growable_buffer();
                for(int y = 0; y < student_count; ++y)
                {
                    char *student_username = get_value(&grade_sheet, user_x, y);
                    write_growable_buffer(&repos_buffer, title, string_len(title));
                    write_constant_string(&repos_buffer, "-");
                    write_growable_buffer(&repos_buffer, student_username, string_len(student_username));
                    write_constant_string(&repos_buffer, "\0");
                }
                StringArray repos = split_null_terminated_strings(&repos_buffer);
                int *issue_numbers = (int *)allocate_memory(student_count * sizeof(*issue_numbers));
                retrieve_issue_numbers_by_title(issue_numbers, &repos, github_token, organization, issue_title);

                // TODO: how to assure all feedbacks in feedback_repo are used (make sure no typo).
                // TODO: clone and open the files.
                git_repository *repo_handle = sync_repository(feedback_repo_dir, feedback_repo_url, 0);
                if(repo_handle)
                {
                    StringArray feedbacks = list_files_with_extension(feedback_dir, "md");
                    StringArray issue_bodies;
                    GrowableBuffer template = read_entire_file(template_path);
                    null_terminate(&template);
                    if(format_feedback_issues(&issue_bodies, template.memory, &grade_sheet, &feedbacks))
                    {
                        StringArray escaped_issue_body = escape_string_array(&issue_bodies);
                        for(int i = 0; i < repos.count; ++i)
                        {
                            if(issue_numbers[i])
                            {
                                edit_issue(github_token, organization, repos.elem[i], issue_title, escaped_issue_body.elem[i], issue_numbers[i]);
                            }
                            else
                            {
                                create_issue(github_token, organization, repos.elem[i], issue_title, escaped_issue_body.elem[i]);
                            }
                        }
                    }
                    else
                    {
                        // TODO: error
                    }
                }
            }
            else
            {
                // TODO: error
            }
        }
        else
        {
            write_error("Key '%s' not found", user_key);
        }
    }
    else
    {
        write_error("Sheet '%s' is empty under spreadsheet id '%s'", title, spreadsheet_id);
    }
    free_sheet(&grade_sheet);
}

static void
do_patch(char *root_dir, char *username, char *github_token, char *organization,
         char *target_repo, char *default_branch, char *patch_branch,
         git_repository *source_repo_handle, GitCommitHash source_end_hash, GrowableBuffer *pull_request_body)
{
    static char target_dir[MAX_PATH_LEN];
    static char target_url[MAX_URL_LEN];
    if(format_string(target_dir, MAX_PATH_LEN, "%s/target_%s", root_dir, target_repo) &&
       format_string(target_url, MAX_PATH_LEN,
                     "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, target_repo))
    {
        git_repository *target_repo_handle = sync_repository(target_dir, target_url, 0);
        if(target_repo_handle)
        {
            GitCommitHash target_begin_hash = get_root_commit(target_repo_handle);
            GitCommitHash source_begin_hash = find_latest_identical_commit(source_repo_handle, source_end_hash,
                                                                           target_repo_handle, target_begin_hash);
            // TODO: handle the deletion of source begin hash
            if(hash_is_valid(&target_begin_hash) && hash_is_valid(&source_begin_hash))
            {
                if(apply_commit_chain(target_repo_handle, patch_branch, target_begin_hash,
                                      source_repo_handle, source_begin_hash, source_end_hash))
                {
                    if(create_pull_request(github_token, organization, target_repo,
                                           patch_branch, default_branch, patch_branch, pull_request_body))
                    {
                        // NOTE: success
                    }
                    else
                    {
                        // TODO: log
                    }
                }
                else
                {
                    // TODO: log
                }
            }
            else
            {
                // TODO: log
            }
        }
        else
        {
            // TODO: log
        }
    }
    else
    {
        // TODO: log
    }
}

static void
patch_homework(char *title, int match_whole_title, char *github_token, char *organization, char *source_repo, char *branch)
{
    char *issue_title = branch;
    GrowableBuffer issue_body = retrieve_issue_body(github_token, organization, source_repo, issue_title);
    GrowableBuffer escaped_issue_body = escape_growable_buffer(&issue_body);
    if(escaped_issue_body.used > 0)
    {
        GitCommitHash source_end_hash = retrieve_latest_commit(github_token, organization, source_repo, branch);
        if(hash_is_valid(&source_end_hash))
        {
            static char username[MAX_URL_LEN];
            static char root_dir[MAX_PATH_LEN];
            static char source_dir[MAX_PATH_LEN];
            static char source_url[MAX_URL_LEN];
            if(retrieve_username(username, MAX_URL_LEN, github_token) &&
               format_string(root_dir, MAX_PATH_LEN, "%s/cache/patch_%s_%s", global_root_dir, source_repo, branch) &&
               format_string(source_dir, MAX_PATH_LEN, "%s/source_%s", root_dir, source_repo) &&
               format_string(source_url, MAX_URL_LEN, "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, source_repo))
            {
                git_repository *source_repo_handle = sync_repository(source_dir, source_url, 0);
                if(match_whole_title)
                {
                    char *target_repo = title;
                    GrowableBuffer default_branch = retrieve_default_branch(github_token, organization, target_repo);
                    if(default_branch.used > 0)
                    {
                        do_patch(root_dir, username, github_token, organization, target_repo, default_branch.memory, branch,
                                 source_repo_handle, source_end_hash, &escaped_issue_body);
                    }
                    free_growable_buffer(&default_branch);
                }
                else
                {
                    StringArray target_repos = retrieve_repos_by_prefix(github_token, organization, title);
                    StringArray default_branchs = retrieve_default_branchs(&target_repos, github_token, organization);
                    for(int i = 0; i < target_repos.count; ++i)
                    {
                        do_patch(root_dir, username, github_token, organization, target_repos.elem[i], default_branchs.elem[i], branch,
                                 source_repo_handle, source_end_hash, &escaped_issue_body);
                    }
                    free_string_array(&default_branchs);
                    free_string_array(&target_repos);
                }
            }
            else
            {
                // TODO: log
            }
        }
        else
        {
            // TODO: error
        }
    }
    else
    {
        // TODO: error
    }
    free_growable_buffer(&escaped_issue_body);
    free_growable_buffer(&issue_body);
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
            static char log_dir[MAX_PATH_LEN];
            static char log_path[MAX_PATH_LEN];
            if(format_string(log_dir, MAX_PATH_LEN, "%s/logs", global_root_dir) &&
               format_string(log_path, MAX_PATH_LEN, "%s/%d-%02d-%02d_%02d-%02d-%02d.log",
                             log_dir, time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                             time.tm_year, time.tm_min, time.tm_sec))
            {
                platform.create_directory(log_dir);
                global_log_file = fopen(log_path, "wb");
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
            "    --log     log to yyyy-mm-dd_hh-mm-ss.log"                              "\n"
            "[command]"                                                                 "\n"
            "    invite-students    invite student into github organization"            "\n"
            "    collect-homework   collect homework and retrieve late submission info" "\n"
            "    grade-homework     grade homework"                                     "\n"
            "    announce-grade     announce grade"                                     "\n"
            "    patch-homework     patch homework"                                     "\n"
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
        char *student_team = config->value[Config_student_team];
        invite_students(input_path, github_token, organization, student_team);
    }
    else if(compare_string(command, "collect-homework"))
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

        char *title = next_arg(parser);
        char *deadline = next_arg(parser);
        char *cutoff_time = next_arg(parser);
        if(!title || !deadline || !cutoff_time || show_command_usage)
        {
            char *usage =
                "usage: hand2 collect-homework [--command-option] ... title deadline cutoff_time"   "\n"
                                                                                                    "\n"
                "[arguments]"                                                                       "\n"
                "    title          title of the homework"                                          "\n"
                "    deadline       deadline of the homework in yyyy-mm-dd"                         "\n"
                "    cutoff_time    max late submission time after deadline (in days)"              "\n"
                "[command-options]"                                                                 "\n";
            write_output(usage);
            return;
        }
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        time_t late_submission_start = parse_iso8601(deadline);
        time_t late_submission_end = late_submission_start + atoi(cutoff_time) * 86400;
        collect_homework(title, github_token, organization, late_submission_start, late_submission_end);
    }
    else if(compare_string(command, "grade-homework"))
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

        char *title = next_arg(parser);
        char *template_repo = next_arg(parser);
        char *deadline = next_arg(parser);
        char *cutoff_time = next_arg(parser);
        if(!title || !template_repo || !deadline || !cutoff_time || show_command_usage)
        {
            char *usage =
                "usage: hand2 grade-homework [--command-option] ... title template_repo deadline cutoff_time"   "\n"
                                                                                                                "\n"
                "[arguments]"                                                                                   "\n"
                "    title             title of the homework"                                                   "\n"
                "    template_repo     homework template repository name"                                       "\n"
                "    deadline          deadline of the homework in yyyy-mm-dd"                                  "\n"
                "    cutoff_time       max late submission time after deadline (in days)"                       "\n"
                "[command-options]"                                                                             "\n";
            write_output(usage);
            return;
        }
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        char *grade_command = config->value[Config_grade_command];
        time_t late_submission_start = parse_iso8601(deadline);
        time_t late_submission_end = late_submission_start + atoi(cutoff_time) * 86400;
        grade_homework(title, template_repo, github_token, organization,
                       late_submission_start, late_submission_end, grade_command);
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
        char *spreadsheet = config->value[Config_spreadsheet];
        char *username_key = config->value[Config_key_username];
        char *google_token = config->value[Config_google_token];
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        announce_grade(title, feedback_repo, google_token, spreadsheet, username_key, github_token, organization);
    }
    else if(compare_string(command, "patch-homework"))
    {
        int show_command_usage = 0;
        int match_whole_title = 0;
        for(char *option = next_option(parser); option; option = next_option(parser))
        {
            if(compare_string(option, "--help")) { show_command_usage = 1; }
            else if(compare_string(option, "--match-whole-title"))
            {
                match_whole_title = 1;
            }
            else
            {
                write_error("unknown option '%s'", option);
            }
        }

        char *title = next_arg(parser);
        char *source_repo = next_arg(parser);
        char *branch = next_arg(parser);
        if(!title || !source_repo || !branch || show_command_usage)
        {
            char *usage =
                "usage: hand2 patch-homework [--command-option] ... title patch-repo branch"    "\n"
                                                                                                "\n"
                "[arguments]"                                                                   "\n"
                "    title                  title of the homework"                              "\n"
                "    patch-repo             the repository used for patch"                      "\n"
                "    branch                 the branch in 'patch-repo' used for patch"          "\n"
                "[command-options]"                                                             "\n"
                "    --match-whole-title    only patch repository with name 'title'"            "\n";
            write_output(usage);
            return;
        }
        char *github_token = config->value[Config_github_token];
        char *organization = config->value[Config_organization];
        patch_homework(title, match_whole_title, github_token, organization, source_repo, branch);
    }
    else
    {
        write_error("unknown command %s", command);
    }

    if(global_log_file) { fclose(global_log_file); }
}

static void
run_hand(int arg_count, char **args)
{
    static char config_path[MAX_PATH_LEN];
    if(!platform.get_root_dir(global_root_dir, MAX_PATH_LEN))
    {
        write_error("unable to get program root dir");
        return;
    }

    if(!format_string(config_path, MAX_PATH_LEN, "%s/config.txt", global_root_dir))
    {
        write_error("buffer too short: %s", config_path);
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
