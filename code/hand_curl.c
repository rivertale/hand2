static size_t
curl_write_func(char *data, size_t elem_size, size_t elem_count, void *userdata)
{
    GrowableBuffer *buffer = (GrowableBuffer *)userdata;
    size_t size = elem_size * elem_count;
    write_growable_buffer(buffer, data, size);
    return size;
}

static CurlGroup
begin_curl_group(int worker_count)
{
    write_log("worker=%d", worker_count);
    CurlGroup result = {0};
    result.handle = curl.curl_multi_init();
    result.worker_count = worker_count;
    result.workers = (CurlWorker *)allocate_memory(worker_count * sizeof(*result.workers));
    clear_memory(result.workers, worker_count * sizeof(*result.workers));
    for(int i = 0; i < result.worker_count; ++i)
        result.workers[i].response = allocate_growable_buffer();

    if(!result.handle)
        write_log("curl_multi_init failed");

    return result;
}

static void
assign_work(CurlGroup *group, int index, char *url, char *header, char *post_type, char *post_data)
{
    write_log("url=%s, type=%s", url, post_type);
    write_log("header=%s", header);
    write_log("data=%s", post_data);
    assert(index < group->worker_count);
    if(!group->handle)
        return;

    int is_post_like = (post_type != 0);
    CURL *handle = curl.curl_easy_init();
    if(handle)
    {
        CURLcode err = 0;
        CURLMcode merr = 0;
        CurlWorker *worker = group->workers + index;
        worker->header_list = curl.curl_slist_append(0, header);
        if(is_post_like)
        {
            if((err = curl.curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, worker->error)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_func)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_WRITEDATA, &worker->response)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, 60000)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_HTTPHEADER, worker->header_list)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_URL, url)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, post_type)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_POSTFIELDS, post_data)) == 0 &&
               (merr = curl.curl_multi_add_handle(group->handle, handle)) == 0)
            {
                worker->handle = handle;
            }
            else
            {
                write_error("%s", merr ? curl.curl_multi_strerror(merr) : curl.curl_easy_strerror(err));
                curl.curl_easy_cleanup(handle);
            }
        }
        else
        {
            if((err = curl.curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, worker->error)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_func)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_WRITEDATA, &worker->response)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, 60000)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_HTTPHEADER, worker->header_list)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_URL, url)) == 0 &&
               (err = curl.curl_easy_setopt(handle, CURLOPT_HTTPGET, 1)) == 0 &&
               (merr = curl.curl_multi_add_handle(group->handle, handle)) == 0)
            {
                worker->handle = handle;
            }
            else
            {
                write_error("%s", merr ? curl.curl_multi_strerror(merr) : curl.curl_easy_strerror(err));
                curl.curl_easy_cleanup(handle);
            }
        }
    }
    else
    {
        write_error("curl_easy_init failed");
    }
}

static void
complete_all_works(CurlGroup *group)
{
    if(!group->handle)
        return;

    int still_running;
    for(;;)
    {
        if(curl.curl_multi_perform(group->handle, &still_running) != 0)
            break;
        if(still_running == 0)
            break;
        if(curl.curl_multi_poll(group->handle, 0, 0, 5000, 0) != 0)
            break;
    }

    for(int i = 0; i < group->worker_count; ++i)
    {
        CurlWorker *worker = group->workers + i;
        curl.curl_easy_getinfo(worker->handle, CURLINFO_RESPONSE_CODE, (long *)&worker->status);
        null_terminate(&worker->response);

        char *url = 0;
        curl.curl_easy_getinfo(worker->handle, CURLINFO_EFFECTIVE_URL, &url);
        write_log("[%d]: status=%d, url=%s", i, worker->status, url);
        if(worker->status >= 400)
            write_log("status %d: %s", worker->status, worker->error);
    }
}

static int
end_curl_group(CurlGroup *group)
{
    int success = 1;
    if(group->handle)
    {
        for(;;)
        {
            int message_in_queue;
            CURLMsg *message = curl.curl_multi_info_read(group->handle, &message_in_queue);
            if(!message)
                break;

            if(message->msg != CURLMSG_DONE || message->data.result != 0)
            {
                success = 0;
                break;
            }
        }

        for(int i = 0; i < group->worker_count; ++i)
        {
            CurlWorker *worker = group->workers + i;
            if(worker->handle)
            {
                curl.curl_multi_remove_handle(group->handle, worker->handle);
                curl.curl_easy_cleanup(worker->handle);
            }
            else
            {
                success = 0;
            }

            if(worker->header_list)
            {
                curl.curl_slist_free_all(worker->header_list);
            }
        }
        curl.curl_multi_cleanup(group->handle);
    }

    for(int i = 0; i < group->worker_count; ++i)
    {
        free_growable_buffer(&group->workers[i].response);
    }
    free_memory(group->workers);
    clear_memory(group, sizeof(*group));
    return success;
}

static int
format_github_header(char *buffer, size_t max_len, char *github_token)
{
    int result = format_string(buffer, max_len,
                               "Accept: application/vnd.github+json"   "\r\n"
                               "Authorization: Bearer %s"              "\r\n"
                               "X-GitHub-Api-Version: 2022-11-28"      "\r\n"
                               "User-Agent: hand",
                               github_token);
    return result;
}

static void
assign_github_get(CurlGroup *group, int index, char *url, char *github_token)
{
    static char header[MAX_REST_HEADER_LEN];
    if(format_github_header(header, MAX_REST_HEADER_LEN, github_token))
    {
        assign_work(group, index, url, header, 0, 0);
    }
}

static void
assign_github_post_like(CurlGroup *group, int index, char *url, char *github_token,
                        char *post_type, char *post_data)
{
    assert(post_type);
    static char header[MAX_REST_HEADER_LEN];
    if(format_github_header(header, MAX_REST_HEADER_LEN, github_token))
    {
        assign_work(group, index, url, header, post_type, post_data);
    }
}

static char *
get_sheet_header(void)
{
    char *result = "Accept: application/json"   "\r\n"
                   "User-Agent: hand";
    return result;
}

static int
format_sheet_url(char *buffer, size_t max_len, char *url, char *google_token)
{
    int result = 0;
    // NOTE: we do not escape the url
    size_t url_len = string_len(url);
    size_t prefix_len = 5; // NOTE: "?key=" or "&key="
    size_t token_len = string_len(google_token);
    if(url_len + prefix_len + token_len < max_len)
    {
        result = 1;
        int has_query = 0;
        for(char *c = url; *c; ++c)
        {
            if(*c == '?')
                has_query = 1;
        }
        copy_memory(buffer,                                    url,                           url_len);
        copy_memory(buffer + url_len,                          has_query ? "&key=" : "?key=", prefix_len);
        copy_memory(buffer + url_len + prefix_len,             google_token,                  token_len);
        copy_memory(buffer + url_len + prefix_len + token_len, "\0",                          1);
    }
    return result;
}

static void
assign_sheet_get(CurlGroup *group, int index, char *url, char *google_token)
{
    static char full_url[MAX_URL_LEN];
    if(format_sheet_url(full_url, MAX_URL_LEN, url, google_token))
    {
        assign_work(group, index, full_url, get_sheet_header(), 0, 0);
    }
}

static char *
get_response(CurlGroup *group, int index)
{
    return group->workers[index].response.memory;
}

static int
github_organization_exists(char *github_token, char *organization)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s", organization))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_get(&group, 0, url, github_token);
        complete_all_works(&group);
        result = end_curl_group(&group);
    }
    return result;
}

static int
github_team_exists(char *github_token, char *organization, char *team)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/teams/%s", organization, team))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_get(&group, 0, url, github_token);
        complete_all_works(&group);
        result = end_curl_group(&group);
    }
    return result;
}

static int
github_repository_exists(char *github_token, char *organization, char *repo)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s", organization, repo))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_get(&group, 0, url, github_token);
        complete_all_works(&group);
        result = end_curl_group(&group);
    }
    return result;
}

static int
retrieve_username(char *out, size_t max_size, char *github_token)
{
    if(max_size > 0 && !out)
        return 0;
    if(max_size > 0)
        *out = 0;

    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/user"))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_get(&group, 0, url, github_token);
        complete_all_works(&group);

        char *response = get_response(&group, 0);
        cJSON *json = cJSON_Parse(response);
        if(!json)
            write_error("JSON corrupted: %s", response);

        cJSON *username = cJSON_GetObjectItemCaseSensitive(json, "login");
        if(cJSON_IsString(username))
        {
            size_t len = min(string_len(username->valuestring), max_size - 1);
            if(out)
            {
                copy_memory(out, username->valuestring, len);
                out[len] = 0;
            }
        }
        result = end_curl_group(&group);
    }
    return result;
}

static void
github_users_exist(int *out, StringArray *users, char *github_token)
{
    clear_memory(out, users->count * sizeof(*out));
#define MAX_WORKER 64
    for(int at = 0; at < users->count; at += MAX_WORKER)
    {
        int worker_count = min(users->count - at, MAX_WORKER);
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            static char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/users/%s", users->elem[i]))
            {
                assign_github_get(&group, i, url, github_token);
            }
        }
        complete_all_works(&group);

        for(int i = 0; i < worker_count; ++i)
        {
            out[at + i] = (group.workers[i].status < 400);
        }
        end_curl_group(&group);
    }
#undef MAX_WORKER
}

static void
retrieve_issue_numbers_by_title(int *out_numbers, StringArray *repos, char *github_token, char *organization, char *title)
{
    clear_memory(out_numbers, repos->count * sizeof(*out_numbers));
#define MAX_WORKER 64
    for(int at = 0; at < repos->count; at += MAX_WORKER)
    {
        int worker_done[MAX_WORKER] = {0};
        int page = 0;
        int worker_count = min(repos->count - at, MAX_WORKER);
        while(!true_for_all(worker_done, worker_count))
        {
            ++page;
            CurlGroup group = begin_curl_group(worker_count);
            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i])
                    continue;

                static char url[MAX_URL_LEN];
                if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/issues?state=all&per_page=100&page=%d",
                                 organization, repos->elem[at + i], page))
                {
                    assign_github_get(&group, i, url, github_token);
                }
            }
            complete_all_works(&group);

            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i])
                    continue;

                char *response = get_response(&group, i);
                cJSON *json = cJSON_Parse(response);
                if(!json)
                    write_error("JSON corrupted: %s", response);

                int issue_count_in_page = 0;
                cJSON *issue_json = 0;
                cJSON_ArrayForEach(issue_json, json)
                {
                    cJSON *id_json = cJSON_GetObjectItemCaseSensitive(issue_json, "number");
                    cJSON *title_json = cJSON_GetObjectItemCaseSensitive(issue_json, "title");
                    if(cJSON_IsNumber(id_json) && cJSON_IsString(title_json) &&
                       compare_string(title_json->valuestring, title))
                    {
                        worker_done[i] = 1;
                        out_numbers[at + i] = id_json->valueint;
                        break;
                    }
                    ++issue_count_in_page;
                }
                cJSON_Delete(json);
                if(issue_count_in_page == 0)
                    worker_done[i] = 1;
            }
            end_curl_group(&group);
        }
    }
#undef MAX_WORKER
}

static void
retrieve_creation_times(time_t *out_times, StringArray *repos, char *github_token, char *organization)
{
    clear_memory(out_times, repos->count * sizeof(*out_times));
#define MAX_WORKER 64
    for(int at = 0; at < repos->count; at += MAX_WORKER)
    {
        int worker_count = min(repos->count - at, MAX_WORKER);
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            static char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s", organization, repos->elem[at + i]))
            {
                assign_github_get(&group, i, url, github_token);
            }
        }
        complete_all_works(&group);

        for(int i = 0; i < worker_count; ++i)
        {
            char *response = get_response(&group, i);
            cJSON *json = cJSON_Parse(response);
            if(!json)
                write_error("JSON corrupted: %s", response);

            cJSON *creation_time = cJSON_GetObjectItemCaseSensitive(json, "created_at");
            if(cJSON_IsString(creation_time))
            {
                out_times[at + i] = parse_time(creation_time->valuestring, TIME_ZONE_UTC0);
            }
        }
        end_curl_group(&group);
    }
#undef MAX_WORKER
}

static StringArray
retrieve_default_branches(StringArray *repos, char *github_token, char *organization)
{
    StringArray result = allocate_string_array();
#define MAX_WORKER 64
    for(int at = 0; at < repos->count; at += MAX_WORKER)
    {
        int worker_count = min(repos->count - at, MAX_WORKER);
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            static char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s", organization, repos->elem[at + i]))
            {
                assign_github_get(&group, i, url, github_token);
            }
        }
        complete_all_works(&group);

        for(int i = 0; i < worker_count; ++i)
        {
            char *response = get_response(&group, i);
            cJSON *json = cJSON_Parse(response);
            if(!json)
                write_error("JSON corrupted: %s", response);

            cJSON *default_branch = cJSON_GetObjectItemCaseSensitive(json, "default_branch");
            if(cJSON_IsString(default_branch))
            {
                append_string_array(&result, default_branch->valuestring);
            }
            else
            {
                // NOTE: must align to the number of repository, so we write a empty string even when we can't find the
                // default branch name
                append_string_array(&result, "");
                write_error("Default branch can't be found for '%s/%s'", organization, repos->elem[i]);
            }
            cJSON_Delete(json);
        }
        end_curl_group(&group);
    }
#undef MAX_WORKER
    assert(result.count == repos->count);
    return result;
}

static void
retrieve_latest_commits(GitCommitHash *out_hash, StringArray *repos, StringArray *branches, char *github_token, char *organization)
{
    int count = repos->count;
    assert(repos->count == branches->count);
    clear_memory(out_hash, count * sizeof(*out_hash));
#define MAX_WORKER 64
    for(int at = 0; at < count; at += MAX_WORKER)
    {
        int worker_count = min(count - at, MAX_WORKER);
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            static char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/branches/%s",
                             organization, repos->elem[at + i], branches->elem[at + i]))
            {
                assign_github_get(&group, i, url, github_token);
            }
        }
        complete_all_works(&group);

        for(int i = 0; i < worker_count; ++i)
        {
            char *response = get_response(&group, i);
            cJSON *json = cJSON_Parse(response);
            if(!json)
                write_error("JSON corrupted: %s", response);

            cJSON *commit = cJSON_GetObjectItemCaseSensitive(json, "commit");
            cJSON *hash = cJSON_GetObjectItemCaseSensitive(commit, "sha");
            if(cJSON_IsString(hash))
            {
                out_hash[at + i] = init_git_commit_hash(hash->valuestring);
            }
            cJSON_Delete(json);
        }
        end_curl_group(&group);
    }
#undef MAX_WORKER
}

static GitCommitHash
retrieve_latest_commit(char *github_token, char *organization, char *repo, char *branch)
{
    GitCommitHash result;
    StringArray repos = {0};
    repos.count = 1;
    repos.elem = &repo;
    if(branch)
    {
        StringArray branches = {0};
        branches.count = 1;
        branches.elem = &branch;
        retrieve_latest_commits(&result, &repos, &branches, github_token, organization);
    }
    else
    {
        StringArray branches = retrieve_default_branches(&repos, github_token, organization);
        assert(branches.count == 1);
        retrieve_latest_commits(&result, &repos, &branches, github_token, organization);
        free_string_array(&branches);
    }
    return result;
}

static StringArray
retrieve_team_members(char *github_token, char *organization, char *team)
{
    StringArray result = allocate_string_array();
    int page = 1;
    for(;;)
    {
        int worker_count = 4;
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/teams/%s/members?page=%d&per_page=100",
                             organization, team, page))
            {
                assign_github_get(&group, i, url, github_token);
            }
            ++page;
        }
        complete_all_works(&group);

        int count_in_page = 0;
        for(int i = 0; i < worker_count; ++i)
        {
            char *response = get_response(&group, i);
            cJSON *json = cJSON_Parse(response);
            if(!json)
                write_error("JSON corrupted: %s", response);

            count_in_page = 0;
            cJSON *member = 0;
            cJSON_ArrayForEach(member, json)
            {
                ++count_in_page;
                cJSON *username = cJSON_GetObjectItemCaseSensitive(member, "login");
                if(cJSON_IsString(username))
                    append_string_array(&result, username->valuestring);
            }
            cJSON_Delete(json);
            if(count_in_page == 0)
                break;
        }
        end_curl_group(&group);
        if(count_in_page == 0)
            break;
    }
    return result;
}

static StringArray
retrieve_existing_invitations(char *github_token, char *organization, char *team)
{
    StringArray result = allocate_string_array();
    int page = 1;
    for(;;)
    {
        int worker_count = 4;
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/teams/%s/invitations?page=%d&per_page=100",
                             organization, team, page))
            {
                assign_github_get(&group, i, url, github_token);
            }
            ++page;
        }
        complete_all_works(&group);

        int count_in_page = 0;
        for(int i = 0; i < worker_count; ++i)
        {
            char *response = get_response(&group, i);
            cJSON *json = cJSON_Parse(response);
            if(!json)
                write_error("JSON corrupted: %s", response);

            count_in_page = 0;
            cJSON *member = 0;
            cJSON_ArrayForEach(member, json)
            {
                ++count_in_page;
                cJSON *username = cJSON_GetObjectItemCaseSensitive(member, "login");
                if(cJSON_IsString(username))
                {
                    append_string_array(&result, username->valuestring);
                }
                else
                {
                    write_error("JSON corrupted: %s", get_response(&group, i));
                }
            }
            cJSON_Delete(json);
            if(count_in_page == 0)
                break;
        }
        end_curl_group(&group);
        if(count_in_page == 0)
            break;
    }
    return result;
}

static StringArray
retrieve_repos_by_prefix(char *github_token, char *organization, char *prefix)
{
    StringArray result = allocate_string_array();
    int page = 1;
    for(;;)
    {
        int worker_count = 8;
        CurlGroup group = begin_curl_group(worker_count);
        for(int i = 0; i < worker_count; ++i)
        {
            char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/repos?page=%d&per_page=100",
                             organization, page))
            {
                assign_github_get(&group, i, url, github_token);
            }
            ++page;
        }
        complete_all_works(&group);

        int count_in_page = 0;
        for(int i = 0; i < worker_count; ++i)
        {
            char *response = get_response(&group, i);
            cJSON *json = cJSON_Parse(response);
            if(!json)
                write_error("JSON corrupted: %s", response);

            count_in_page = 0;
            cJSON *repo = 0;
            cJSON_ArrayForEach(repo, json)
            {
                ++count_in_page;
                cJSON *repo_name = cJSON_GetObjectItemCaseSensitive(repo, "name");
                if(cJSON_IsString(repo_name) && compare_substring(repo_name->valuestring, prefix, string_len(prefix)))
                {
                    append_string_array(&result, repo_name->valuestring);
                }
            }
            cJSON_Delete(json);
            if(count_in_page == 0)
                break;
        }
        end_curl_group(&group);
        if(count_in_page == 0)
            break;
    }
    return result;
}

// NOTE:
// - we don't believe commit time since it can be modifed by students.
// - we return creation time of the repository and the latest commit as the last resort, this will
//   happen when there aren't any push events before the cutoff (the creation of the repo is not a
//   push event). if the students never push, the latest commit is the commit they accept the homework.

// NOTE: push events only retain for 90 days, if the student only push 90 days before you collect and
// grade it, it will assume the student pushes the latest commit in Day 1 (when accepting homework).
static void
retrieve_pushes_before_cutoff(GitCommitHash *out_hash, time_t *out_push_time, StringArray *repos, StringArray *req_branches,
                              char *github_token, char *organization, time_t cutoff)
{
    clear_memory(out_hash, repos->count * sizeof(*out_hash));
    clear_memory(out_push_time, repos->count * sizeof(*out_push_time));
    time_t *last_resort_push_times = (time_t *)allocate_memory(repos->count * sizeof(*last_resort_push_times));
    GitCommitHash *last_resort_hashes = (GitCommitHash *)allocate_memory(repos->count * sizeof(*last_resort_hashes));
    retrieve_creation_times(last_resort_push_times, repos, github_token, organization);
    retrieve_latest_commits(last_resort_hashes, repos, req_branches, github_token, organization);

#define MAX_WORKER 64
	for(int at = 0; at < repos->count; at += MAX_WORKER)
    {
        int page = 1;
        int worker_done[MAX_WORKER] = {0};
        int worker_count = min(repos->count - at, MAX_WORKER);
        while(!true_for_all(worker_done, worker_count))
        {
            CurlGroup group = begin_curl_group(worker_count);
            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i])
                    continue;

                static char url[MAX_URL_LEN];
                if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/events?page=%d&per_page=100",
                                 organization, repos->elem[at + i], page))
                {
                    assign_github_get(&group, i, url, github_token);
                }
            }
            complete_all_works(&group);

            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i])
                    continue;

                int event_count_in_page = 0;
                static char req_ref[256];
                if(format_string(req_ref, sizeof(req_ref), "refs/heads/%s", req_branches->elem[at + i]))
                {
                    char *response = get_response(&group, i);
                    cJSON *json = cJSON_Parse(response);
                    if(!json)
                        write_error("JSON corrupted: %s", response);

                    cJSON *event = 0;
                    cJSON_ArrayForEach(event, json)
                    {
                        ++event_count_in_page;
                        cJSON *event_type = cJSON_GetObjectItemCaseSensitive(event, "type");
                        cJSON *created_at = cJSON_GetObjectItemCaseSensitive(event, "created_at");
                        cJSON *payload = cJSON_GetObjectItemCaseSensitive(event, "payload");
                        cJSON *push_hash = cJSON_GetObjectItemCaseSensitive(payload, "head");
                        cJSON *ref_type = cJSON_GetObjectItemCaseSensitive(payload, "ref_type");
                        cJSON *ref = cJSON_GetObjectItemCaseSensitive(payload, "ref");
                        cJSON *default_branch = cJSON_GetObjectItemCaseSensitive(payload, "master_branch");
                        if(cJSON_IsString(event_type) && compare_string(event_type->valuestring, "PushEvent") &&
                           cJSON_IsString(created_at) && cJSON_IsString(push_hash) &&
                           cJSON_IsString(ref) && compare_string(ref->valuestring, req_ref))
                        {
                            time_t push_time = parse_time(created_at->valuestring, TIME_ZONE_UTC0);
                            if(push_time < cutoff)
                            {
                                worker_done[i] = 1;
                                out_push_time[at + i] = push_time;
                                out_hash[at + i] = init_git_commit_hash(push_hash->valuestring);
                                break;
                            }
                        }
                        else if(cJSON_IsString(event_type) && compare_string(event_type->valuestring, "CreateEvent") &&
                                cJSON_IsString(created_at) &&
                                cJSON_IsString(ref_type) && compare_string(ref_type->valuestring, "branch") &&
                                // NOTE: weird github api, create event's ref is 'branch',
                                // while push event's ref is 'refs/heads/branch'
                                cJSON_IsString(ref) && compare_string(ref->valuestring, req_branches->elem[at + i]))
                        {
                            time_t push_time = parse_time(created_at->valuestring, TIME_ZONE_UTC0);
                            if(push_time < cutoff)
                            {
                                worker_done[i] = 1;
                                out_push_time[at + i] = push_time;
                                out_hash[at + i] = last_resort_hashes[at + i];
                                break;
                            }
                        }
                        else if(cJSON_IsString(event_type) && compare_string(event_type->valuestring, "CreateEvent") &&
                                cJSON_IsString(created_at) &&
                                cJSON_IsString(ref_type) && compare_string(ref_type->valuestring, "repository") &&
                                cJSON_IsString(default_branch) &&
                                compare_string(default_branch->valuestring, req_branches->elem[at + i]))
                        {
                            time_t push_time = parse_time(created_at->valuestring, TIME_ZONE_UTC0);
                            if(push_time < cutoff)
                            {
                                worker_done[i] = 1;
                                out_push_time[at + i] = push_time;
                                out_hash[at + i] = last_resort_hashes[at + i];
                                break;
                            }
                        }
                    }
                }

                if(!out_push_time[at + i])
                {
                    out_push_time[at + i] = last_resort_push_times[at + i];
                    out_hash[at + i] = last_resort_hashes[at + i];
                }

                if(event_count_in_page == 0)
                    worker_done[i] = 1;
            }
            end_curl_group(&group);
            ++page;
        }
    }
#undef MAX_WORKER
}

static int
invite_user_to_team(char *github_token, char *username, char *organization, char *team)
{
    int result = 0;
    char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/teams/%s/memberships/%s",
                     organization, team, username))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_post_like(&group, 0, url, github_token, "PUT", "{\"role\":\"member\"}");
        complete_all_works(&group);
        result = end_curl_group(&group);
    }
    return result;
}

static int
create_issue(char *github_token, char *organization, char *repo, char *title, char *body)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/issues", organization, repo))
    {
        GrowableBuffer escaped_body = escape_string(body);
        GrowableBuffer post_data = allocate_growable_buffer();
        write_constant_string(&post_data, "{\"title\":\"");
        write_growable_buffer(&post_data, title, string_len(title));
        write_constant_string(&post_data, "\",\"body\":\"");
        write_growable_buffer(&post_data, escaped_body.memory, escaped_body.used);
        write_constant_string(&post_data, "\",\"state\":\"open\"}");

        CurlGroup group = begin_curl_group(1);
        assign_github_post_like(&group, 0, url, github_token, "POST", post_data.memory);
        complete_all_works(&group);
        if(end_curl_group(&group))
        {
            result = 1;
        }
        free_growable_buffer(&post_data);
        free_growable_buffer(&escaped_body);
    }
    return result;
}

static int
edit_issue(char *github_token, char *organization, char *repo, char *title, char *body, int issue_number)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/issues/%d", organization, repo, issue_number))
    {
        GrowableBuffer escaped_body = escape_string(body);
        GrowableBuffer post_data = allocate_growable_buffer();
        write_constant_string(&post_data, "{\"title\":\"");
        write_growable_buffer(&post_data, title, string_len(title));
        write_constant_string(&post_data, "\",\"body\":\"");
        write_growable_buffer(&post_data, escaped_body.memory, escaped_body.used);
        write_constant_string(&post_data, "\",\"state\":\"open\"}");

        CurlGroup group = begin_curl_group(1);
        assign_github_post_like(&group, 0, url, github_token, "PATCH", post_data.memory);
        complete_all_works(&group);
        if(end_curl_group(&group))
        {
            result = 1;
        }
        free_growable_buffer(&post_data);
        free_growable_buffer(&escaped_body);
    }
    return result;
}

static int
google_token_is_valid(char *google_token)
{
    char *url = "https://sheets.googleapis.com/v4/spreadsheets/0";
    CurlGroup group = begin_curl_group(1);
    assign_sheet_get(&group, 0, url, google_token);
    complete_all_works(&group);
    int status = group.workers[0].status;
    end_curl_group(&group);
    return (status == 404);
}

static int
retrieve_spreadsheet(StringArray *out, char *google_token, char *spreadsheet_id)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    *out = allocate_string_array();
    if(format_string(url, MAX_URL_LEN, "https://sheets.googleapis.com/v4/spreadsheets/%s", spreadsheet_id))
    {
        CurlGroup group = begin_curl_group(1);
        assign_sheet_get(&group, 0, url, google_token);
        complete_all_works(&group);

        char *response = get_response(&group, 0);
        cJSON *json = cJSON_Parse(response);
        if(!json)
            write_error("JSON corrupted: %s", response);

        cJSON *sheets = cJSON_GetObjectItemCaseSensitive(json, "sheets");
        cJSON *sheet = 0;
        cJSON_ArrayForEach(sheet, sheets)
        {
            cJSON *properties = cJSON_GetObjectItemCaseSensitive(sheet, "properties");
            cJSON *title = cJSON_GetObjectItemCaseSensitive(properties, "title");
            if(cJSON_IsString(title))
                append_string_array(out, title->valuestring);
        }
        result = end_curl_group(&group);
    }
    return result;
}

static Sheet
retrieve_sheet(char *google_token, char *spreadsheet, char *name)
{
    Sheet result = {0};
    static char hex_char[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    static char url[MAX_URL_LEN];

    GrowableBuffer escaped_name = allocate_growable_buffer();
    for(unsigned char *c = (unsigned char *)name; *c; ++c)
    {
        if(('A' <= *c && *c <= 'Z') || ('a' <= *c && *c <= 'z') || ('0' <= *c && *c <= '9') ||
           *c == '-' || *c == '_' || *c == '.' || *c == '~')
        {
            write_growable_buffer(&escaped_name, c, 1);
        }
        else
        {
            write_constant_string(&escaped_name, "%");
            write_growable_buffer(&escaped_name, &hex_char[(*c >> 4) & 0xf], 1);
            write_growable_buffer(&escaped_name, &hex_char[(*c >> 0) & 0xf], 1);
        }
    }

    if(format_string(url, MAX_URL_LEN,
                     "https://sheets.googleapis.com/v4/spreadsheets/%s/values/'%s'", spreadsheet, escaped_name.memory))
    {
        CurlGroup group = begin_curl_group(1);
        assign_sheet_get(&group, 0, url, google_token);
        complete_all_works(&group);

        char *response = get_response(&group, 0);
        cJSON *json = cJSON_Parse(response);
        if(!json)
            write_error("JSON corrupted: %s", response);

        cJSON *row = 0;
        cJSON *rows = cJSON_GetObjectItemCaseSensitive(json, "values");
        int width = 0;
        int height = cJSON_GetArraySize(rows) - 1;
        cJSON_ArrayForEach(row, rows)
        {
            int row_width = cJSON_GetArraySize(row);
            width = max(width, row_width);
        }

        if(width > 0 && height > 0)
        {
            result = allocate_sheet(width, height);
            // NOTE: fill all string first to make sure the pointers don't change
            cJSON_ArrayForEach(row, rows)
            {
                cJSON *cell = 0;
                cJSON_ArrayForEach(cell, row)
                {
                    if(cJSON_IsString(cell))
                        write_growable_buffer(&result.content, cell->valuestring, string_len(cell->valuestring));

                    write_constant_string(&result.content, "\0");
                }

                for(int x = cJSON_GetArraySize(row); x < width; ++x)
                    write_constant_string(&result.content, "\0");
            }

            char *at = result.content.memory;
            for(int i = 0; i < width; ++i)
            {
                result.labels[i] = at;
                at += string_len(at) + 1;
            }

            for(int i = 0; i < width * height; ++i)
            {
                result.values[i] = at;
                at += string_len(at) + 1;
            }
        }
        else
        {
            write_log("sheet '%s:%s' is empty", spreadsheet, name);
        }
        end_curl_group(&group);
    }
    free_growable_buffer(&escaped_name);
    return result;
}
