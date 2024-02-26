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
    CurlGroup result = {0};
    result.handle = curl.curl_multi_init();
    result.worker_count = worker_count;
    result.workers = (CurlWorker *)allocate_memory(worker_count * sizeof(*result.workers));
    clear_memory(result.workers, worker_count * sizeof(*result.workers));
    for(int i = 0; i < result.worker_count; ++i)
    {
        result.workers[i].response = allocate_growable_buffer();
    }
    
    if(!result.handle)
    {
        write_error("curl_multi_init failed");
    }
    return result;
}

static void
assign_work(CurlGroup *group, int index, char *url, char *header, char *post_type, char *post_data)
{
    assert(index < group->worker_count);
    if(!group->handle) return;
    
    int is_post_like = (post_type != 0);
    CURL *handle = curl.curl_easy_init();
    if(handle)
    {
        CurlWorker *worker = group->workers + index;
        worker->header_list = curl.curl_slist_append(0, header);
        if(is_post_like)
        {
            if((curl.curl_easy_setopt(handle, CURLOPT_CAINFO, global_certificate_path) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, worker->error) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_func) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_WRITEDATA, &worker->response) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_HTTPHEADER, worker->header_list) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_URL, url) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, post_type) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_POSTFIELDS, post_data) == 0) &&
               (curl.curl_multi_add_handle(group->handle, handle) == 0))
            {
                worker->handle = handle;
            }
            else
            {
                // TODO: log
                curl.curl_easy_cleanup(handle);
            }
        }
        else
        {
            if((curl.curl_easy_setopt(handle, CURLOPT_CAINFO, global_certificate_path) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, worker->error) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_func) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_WRITEDATA, &worker->response) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_HTTPHEADER, worker->header_list) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_URL, url) == 0) &&
               (curl.curl_easy_setopt(handle, CURLOPT_HTTPGET, 1) == 0) &&
               (curl.curl_multi_add_handle(group->handle, handle) == 0))
            {
                worker->handle = handle;
            }
            else
            {
                // TODO: log
                curl.curl_easy_cleanup(handle);
            }
        }
    }
    else
    {
        // TODO: log
    }
}

static void 
complete_all_works(CurlGroup *group)
{
    if(!group->handle) return;
    
    int still_running;
    for(;;)
    {
        if(curl.curl_multi_perform(group->handle, &still_running) != 0) break;
        if(still_running == 0) break;
        if(curl.curl_multi_poll(group->handle, 0, 0, 5000, 0) != 0) break;
    }
    
    for(int i = 0; i < group->worker_count; ++i)
    {
        null_terminate(&group->workers[i].response);
    }
}

static int 
end_curl_group(CurlGroup *group)
{
    int no_error = 1;
    if(group->handle)
    {
        for(;;)
        {
            int message_in_queue;
            CURLMsg *message = curl.curl_multi_info_read(group->handle, &message_in_queue);
            if(!message) break;
            
            if(message->msg != CURLMSG_DONE || message->data.result != 0)
            {
                no_error = 0;
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
                no_error = 0;
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
    return no_error;
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
            if(*c == '?') { has_query = 1; }
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
github_user_exists(char *github_token, char *username)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/users/%s", username))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_get(&group, 0, url, github_token);
        complete_all_works(&group);
        result = end_curl_group(&group);
    }
    return result;
}

static int 
retrieve_username(char *out_username, size_t max_username_len, char *github_token)
{
    if(max_username_len > 0 && !out_username) return 0;
    if(max_username_len > 0) { *out_username = 0; }
    
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/user"))
    {
        CurlGroup group = begin_curl_group(1);
        assign_github_get(&group, 0, url, github_token);
        complete_all_works(&group);
        
        cJSON *json = cJSON_Parse(get_response(&group, 0));
        cJSON *username = cJSON_GetObjectItemCaseSensitive(json, "login");
        if(cJSON_IsString(username))
        {
            size_t username_len = min(string_len(username->valuestring), max_username_len - 1);
            if(out_username)
            {
                copy_memory(out_username, username->valuestring, username_len);
                out_username[username_len] = 0;
            }
        }
        result = end_curl_group(&group);
    }
    return result;
}

static void
retrieve_issue_numbers_by_title(int *out_numbers, StringArray *repos, char *github_token, char *organization, char *title)
{
    clear_memory(out_numbers, repos->count * sizeof(*out_numbers));
#define MAX_WORKER 64
    for(int at = 0; at < repos->count; at += MAX_WORKER)
    {
        int worker_done[MAX_WORKER] = {0};
        int prev_pages[MAX_WORKER] = {0};
        int worker_count = min(repos->count - at, MAX_WORKER);
        while(!true_for_all(worker_done, worker_count))
        {
            CurlGroup group = begin_curl_group(worker_count);
            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i]) continue;
                static char url[MAX_URL_LEN];
                // TODO: aren't prev_pages always the same for all workers?
                if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/issues/?state=all&per_page=100&page=%d", 
                                 organization, repos->elem[at + i], ++prev_pages[i]))
                {
                    assign_github_get(&group, i, url, github_token);
                }
            }
            complete_all_works(&group);
            
            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i]) continue;
                int issue_count_in_page = 0;
                cJSON *issue_json = 0;
                cJSON *json = cJSON_Parse(get_response(&group, i));
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
                if(issue_count_in_page == 0) { worker_done[i] = 1; }
                cJSON_Delete(json);
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
            cJSON *json = cJSON_Parse(get_response(&group, i));
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
retrieve_default_branchs(StringArray *repos, char *github_token, char *organization)
{
    GrowableBuffer buffer = allocate_growable_buffer();
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
            cJSON *json = cJSON_Parse(get_response(&group, i));
            cJSON *default_branch = cJSON_GetObjectItemCaseSensitive(json, "default_branch");
            if(cJSON_IsString(default_branch))
            {
                size_t len = string_len(default_branch->valuestring);
                write_growable_buffer(&buffer, default_branch->valuestring, len);
            }
            cJSON_Delete(json);
            
            // NOTE: must align to the number of repository, so we write a empty string even when we can't find the 
            // default branch name
            write_growable_buffer(&buffer, "\0", 1);
        }
        end_curl_group(&group);
    }
#undef MAX_WORKER
    return split_null_terminated_strings(&buffer);
}

static void
retrieve_latest_commits(GitCommitHash *out_hash, StringArray *repos, StringArray *branchs, char *github_token, char *organization)
{
    int count = repos->count;
    assert(repos->count == branchs->count);
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
                             organization, repos->elem[at + i], branchs->elem[at + i]))
            {
                assign_github_get(&group, i, url, github_token);
            }
        }
        complete_all_works(&group);
        
        for(int i = 0; i < worker_count; ++i)
        {
            cJSON *json = cJSON_Parse(get_response(&group, i));
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

static GrowableBuffer
retrieve_default_branch(char *github_token, char *organization, char *repo)
{
    GrowableBuffer result = allocate_growable_buffer();
    StringArray repos = {0};
    repos.count = 1;
    repos.elem = &repo;
    StringArray default_branchs = retrieve_default_branchs(&repos, github_token, organization);
    if(default_branchs.count > 0)
    {
        size_t len = string_len(default_branchs.elem[0]);
        write_growable_buffer(&result, default_branchs.elem[0], len);
    }
    free_string_array(&default_branchs);
    return result;
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
        StringArray branchs = {0};
        branchs.count = 1;
        branchs.elem = &branch;
        retrieve_latest_commits(&result, &repos, &branchs, github_token, organization);
    }
    else
    {
        StringArray branchs = retrieve_default_branchs(&repos, github_token, organization);
        assert(branchs.count == 1);
        retrieve_latest_commits(&result, &repos, &branchs, github_token, organization);
        free_string_array(&branchs);
    }
    return result;
}

static StringArray
retrieve_users_in_team(char *github_token, char *organization, char *team)
{
    GrowableBuffer buffer = allocate_growable_buffer();
    int page = 1;
    for(;;)
    {
        int worker_count = 4;
        CurlGroup group = begin_curl_group(worker_count);
        for(int index = 0; index < worker_count; ++index)
        {
            char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/teams/%s/members?page=%d&per_page=100", 
                             organization, team, page))
            {
                assign_github_get(&group, index, url, github_token);
            }
            ++page;
        }
        complete_all_works(&group);
        
        int count_in_page = 0;
        for(int index = 0; index < worker_count; ++index)
        {
            count_in_page = 0;
            cJSON *json = cJSON_Parse(get_response(&group, index));
            cJSON *member = 0;
            cJSON_ArrayForEach(member, json)
            {
                ++count_in_page;
                cJSON *username = cJSON_GetObjectItemCaseSensitive(member, "login");
                if(cJSON_IsString(username))
                {
                    size_t len = string_len(username->valuestring);
                    write_growable_buffer(&buffer, username->valuestring, len);
                    null_terminate(&buffer);
                }
            }
            cJSON_Delete(json);
            if(count_in_page == 0) break;
        }
        end_curl_group(&group);
        if(count_in_page == 0) break;
    }
    return split_null_terminated_strings(&buffer);
}

static StringArray
retrieve_existing_invitations(char *github_token, char *organization, char *team)
{
    GrowableBuffer buffer = allocate_growable_buffer();
    int page = 1;
    for(;;)
    {
        int worker_count = 4;
        CurlGroup group = begin_curl_group(worker_count);
        for(int index = 0; index < worker_count; ++index)
        {
            char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/teams/%s/invitations?page=%d&per_page=100", 
                             organization, team, page))
            {
                assign_github_get(&group, index, url, github_token);
            }
            ++page;
        }
        complete_all_works(&group);
        
        int count_in_page = 0;
        for(int index = 0; index < worker_count; ++index)
        {
            count_in_page = 0;
            cJSON *json = cJSON_Parse(get_response(&group, index));
            cJSON *member = 0;
            cJSON_ArrayForEach(member, json)
            {
                ++count_in_page;
                cJSON *username = cJSON_GetObjectItemCaseSensitive(member, "login");
                if(cJSON_IsString(username))
                {
                    size_t len = string_len(username->valuestring);
                    write_growable_buffer(&buffer, username->valuestring, len);
                    null_terminate(&buffer);
                }
            }
            cJSON_Delete(json);
            if(count_in_page == 0) break;
        }
        end_curl_group(&group);
        if(count_in_page == 0) break;
    }
    return split_null_terminated_strings(&buffer);
}

static StringArray
retrieve_repos_by_prefix(char *github_token, char *organization, char *prefix)
{
    GrowableBuffer buffer = allocate_growable_buffer();
    int page = 1;
    for(;;)
    {
        int worker_count = 8;
        CurlGroup group = begin_curl_group(worker_count);
        for(int index = 0; index < worker_count; ++index)
        {
            char url[MAX_URL_LEN];
            if(format_string(url, MAX_URL_LEN, "https://api.github.com/orgs/%s/repos?page=%d&per_page=100", 
                             organization, page))
            {
                assign_github_get(&group, index, url, github_token);
            }
            ++page;
        }
        complete_all_works(&group);
        
        int count_in_page = 0;
        for(int index = 0; index < worker_count; ++index)
        {
            count_in_page = 0;
            cJSON *json = cJSON_Parse(get_response(&group, index));
            cJSON *repo = 0;
            cJSON_ArrayForEach(repo, json)
            {
                ++count_in_page;
                cJSON *repo_name = cJSON_GetObjectItemCaseSensitive(repo, "name");
                if(cJSON_IsString(repo_name) && compare_substring(repo_name->valuestring, prefix, string_len(prefix)))
                {
                    size_t len = string_len(repo_name->valuestring);
                    write_growable_buffer(&buffer, repo_name->valuestring, len);
                    null_terminate(&buffer);
                }
            }
            cJSON_Delete(json);
            if(count_in_page == 0) break;
        }
        end_curl_group(&group);
        if(count_in_page == 0) break;
    }
    return split_null_terminated_strings(&buffer);
}

static GrowableBuffer
retrieve_issue_body(char *github_token, char *organization, char *revise_repo, char *issue_title)
{
	GrowableBuffer result = allocate_growable_buffer();
	int found = 0;
    int page = 1;
	for(;;)
	{
		int worker_count = 8;
		CurlGroup group = begin_curl_group(worker_count);
		for(int i = 0; i < worker_count; ++i)
		{
			static char url[MAX_URL_LEN];
			if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/issues?page=%d&per_page=100", 
						     organization, revise_repo, page))
			{
				assign_github_get(&group, i, url, github_token);
			}
			else
			{
				// TODO: log
			}
            ++page;
		}
		complete_all_works(&group);
		
		int issue_count_in_page = 0;
		for(int i = 0; i < worker_count; ++i)
		{
			issue_count_in_page = 0;
			cJSON *json = cJSON_Parse(get_response(&group, i));
			cJSON *issue = 0;
			cJSON_ArrayForEach(issue, json)
			{
				++issue_count_in_page;
				cJSON *title = cJSON_GetObjectItemCaseSensitive(issue, "title");
				cJSON *body = cJSON_GetObjectItemCaseSensitive(issue, "body");
				if(cJSON_IsString(title) && compare_string(title->valuestring, issue_title) &&
				   cJSON_IsString(body))
				{
                    found = 1;
					write_growable_buffer(&result, body->valuestring, string_len(body->valuestring));
					break;
				}
			}
            cJSON_Delete(json);
			
			if(found || issue_count_in_page == 0) break;
		}
		end_curl_group(&group);
		if(found || issue_count_in_page == 0) break;
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
retrieve_pushes_before_cutoff(GitCommitHash *out_hash, time_t *out_push_time, StringArray *repos, StringArray *requested_branchs, 
                              char *github_token, char *organization, time_t cutoff)
{
    clear_memory(out_hash, repos->count * sizeof(*out_hash));
    clear_memory(out_push_time, repos->count * sizeof(*out_push_time));
    time_t *last_resort_push_times = (time_t *)allocate_memory(repos->count * sizeof(*last_resort_push_times));
    GitCommitHash *last_resort_hashes = (GitCommitHash *)allocate_memory(repos->count * sizeof(*last_resort_hashes));
    retrieve_creation_times(last_resort_push_times, repos, github_token, organization);
    retrieve_latest_commits(last_resort_hashes, repos, requested_branchs, github_token, organization);
    
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
                if(worker_done[i]) continue;
                char url[MAX_URL_LEN];
                if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/events?page=%d&per_page=100",
                                 organization, repos->elem[at + i], page))
                {
                    assign_github_get(&group, i, url, github_token);
                }
            }
            complete_all_works(&group);
            
            for(int i = 0; i < worker_count; ++i)
            {
                if(worker_done[i]) continue;
                
                int event_count_in_page = 0;
                static char requested_ref[256];
                if(format_string(requested_ref, sizeof(requested_ref), "refs/heads/%s", requested_branchs->elem[at + i]))
                {
                    cJSON *json = cJSON_Parse(get_response(&group, i));
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
                           cJSON_IsString(ref) && compare_string(ref->valuestring, requested_ref))
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
                                cJSON_IsString(ref) && compare_string(ref->valuestring, requested_branchs->elem[at + i]))
                        {
                            time_t push_time = parse_time(created_at->valuestring, TIME_ZONE_UTC0);
                            if(push_time < cutoff)
                            {
                                worker_done[i] = 1;
                                out_push_time[at + i] = push_time;
                                out_hash[at + i] = init_git_commit_hash(last_resort_hashes[at + i].string);
                                break;
                            }
                        }
                        else if(cJSON_IsString(event_type) && compare_string(event_type->valuestring, "CreateEvent") &&
                                cJSON_IsString(created_at) &&
                                cJSON_IsString(ref_type) && compare_string(ref_type->valuestring, "repository") &&
                                cJSON_IsString(default_branch) && 
                                compare_string(default_branch->valuestring, requested_branchs->elem[at + i]))
                        {
                            time_t push_time = parse_time(created_at->valuestring, TIME_ZONE_UTC0);
                            if(push_time < cutoff)
                            {
                                worker_done[i] = 1;
                                out_push_time[at + i] = push_time;
                                out_hash[at + i] = init_git_commit_hash(last_resort_hashes[at + i].string);
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
                
                if(event_count_in_page == 0) { worker_done[i] = 1; }
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
create_pull_request(char *github_token, char *organization, char *repo, 
                    char *branch, char *branch_to_merge, char *title, GrowableBuffer *body)
{
    int result = 0;
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://api.github.com/repos/%s/%s/pulls", organization, repo))
    {
        GrowableBuffer post_data = allocate_growable_buffer();
        write_constant_string(&post_data, "{");
        write_constant_string(&post_data, "\"title\":\"[PATCH] ");
        write_growable_buffer(&post_data, title, string_len(title));
        write_constant_string(&post_data, "\",\"body\":\"");
        write_growable_buffer(&post_data, body->memory, body->used);
        write_constant_string(&post_data, "\",\"head\":\"");
        write_growable_buffer(&post_data, branch, string_len(branch));
        write_constant_string(&post_data, "\",\"base\":\"");
        write_growable_buffer(&post_data, branch_to_merge, string_len(branch_to_merge));
        write_constant_string(&post_data, "\"}\0");
        
        CurlGroup group = begin_curl_group(1);
        assign_github_post_like(&group, 0, url, github_token, "POST", post_data.memory);
        complete_all_works(&group);
        if(end_curl_group(&group))
        {
            result = 1;
        }
    }
    else
    {
        // TODO: log
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
        GrowableBuffer post_data = allocate_growable_buffer();
        write_constant_string(&post_data, "{\"title\":\"");
        write_growable_buffer(&post_data, title, string_len(title));
        write_constant_string(&post_data, "\",\"body\":\"");
        write_growable_buffer(&post_data, body, string_len(body));
        write_constant_string(&post_data, "\",\"state\":\"open\"}");
        null_terminate(&post_data);
        
        CurlGroup group = begin_curl_group(1);
        assign_github_post_like(&group, 0, url, github_token, "POST", post_data.memory);
        complete_all_works(&group);
        if(end_curl_group(&group))
        {
            result = 1;
        }
    }
    else
    {
        // TODO: log
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
        GrowableBuffer post_data = allocate_growable_buffer();
        write_constant_string(&post_data, "{\"title\":\"");
        write_growable_buffer(&post_data, title, string_len(title));
        write_constant_string(&post_data, "\",\"body\":\"");
        write_growable_buffer(&post_data, body, string_len(body));
        write_constant_string(&post_data, "\",\"state\":\"open\"}");
        null_terminate(&post_data);
        
        CurlGroup group = begin_curl_group(1);
        assign_github_post_like(&group, 0, url, github_token, "PATCH", post_data.memory);
        complete_all_works(&group);
        if(end_curl_group(&group))
        {
            result = 1;
        }
    }
    else
    {
        // TODO: log
    }
    return result;
}

static Sheet
retrieve_sheet(char *google_token, char *spreadsheet, char *sheet)
{
    Sheet result = {0};
    static char url[MAX_URL_LEN];
    if(format_string(url, MAX_URL_LEN, "https://sheets.googleapis.com/v4/spreadsheets/%s/values/'%s'", spreadsheet, sheet))
    {
        CurlGroup group = begin_curl_group(1);
        assign_sheet_get(&group, 0, url, google_token);
        complete_all_works(&group);
        
        cJSON *json = cJSON_Parse(get_response(&group, 0));
        cJSON *rows = cJSON_GetObjectItemCaseSensitive(json, "values");
        cJSON *first_row = cJSON_GetArrayItem(rows, 0);
        int width = cJSON_GetArraySize(first_row);
        int height = cJSON_GetArraySize(rows) - 1;
        if(width > 0 && height > 0)
        {
            result = allocate_sheet(width, height);
            // NOTE: fill all string first to make sure the pointers don't change
            cJSON *row = 0;
            cJSON_ArrayForEach(row, rows)
            {
                assert(cJSON_GetArraySize(row) == width);
                cJSON *cell = 0;
                cJSON_ArrayForEach(cell, row)
                {
                    if(cJSON_IsString(cell))
                    {
                        write_growable_buffer(&result.content, cell->valuestring, string_len(cell->valuestring));
                    }
                    write_constant_string(&result.content, "\0");
                }
            }
            
            char *at = result.content.memory;
            for(int i = 0; i < width; ++i)
            {
                result.keys[i] = at;
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
            // TODO: log
        }
        end_curl_group(&group);
    }
    else
    {
        // TODO: log
    }
    return result;
}
