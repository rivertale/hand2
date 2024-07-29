static int
git_reset_callback(git_checkout_notify_t why, const char *path,
                   const git_diff_file *baseline, const git_diff_file *target, const git_diff_file *workdir, void *payload)
{
    (void)why;
    (void)baseline;
    (void)target;
    (void)workdir;

    char *parent = (char *)payload;
    static char full_path[MAX_PATH_LEN];
    format_string(full_path, MAX_PATH_LEN, "%s/%s", parent, path);
    if(platform.delete_file(full_path))
    {
        // NOTE: remove successfully
    }
    return 0;
}

static int
git_certificate_no_check_callback(git_cert *cert, int valid, const char *host, void *payload)
{
    (void)cert;
    (void)valid;
    (void)host;
    (void)payload;
    return 0;
}

static int
git_acquire_credential_callback(git_credential **out, const char *url, const char *username_from_url,
                                unsigned int allowed_types, void *payload)
{
    (void)url;
    (void)username_from_url;
    char **ptr = (char **)payload;
    char *username = ptr[0];
    char *github_token = ptr[1];
    if(allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT)
    {
        if(git2.git_credential_userpass_plaintext_new(out, username, github_token) == 0)
            return 0;
    }
    else if(allowed_types & GIT_CREDENTIAL_DEFAULT)
    {
        if(git2.git_credential_default_new(out) == 0)
            return 0;
    }
    else if(allowed_types & GIT_CREDENTIAL_USERNAME)
    {
        if(git2.git_credential_username_new(out, username) == 0)
            return 0;
    }
    return GIT_PASSTHROUGH;
}

static int
clone_repository(char *dir, char *username, char *github_token, char *organization, char *repo, GitCommitHash hash)
{
    int success = 0;
    assert(hash_is_valid(&hash));

    static char url[MAX_URL_LEN];
    format_string(url, MAX_URL_LEN, "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, repo);
    platform.delete_directory(g_git_temporary_dir);

    int clone_and_reset_success = 0;
    git_clone_options clone_options = GIT_CLONE_OPTIONS_INIT;
    clone_options.fetch_opts.callbacks.certificate_check = git_certificate_no_check_callback;
    git_repository *repository;
    if(git2.git_clone(&repository, url, g_git_temporary_dir, &clone_options) == 0)
    {
        git_oid commit_oid;
        git_commit *commit;
        git_checkout_options reset_options = GIT_CHECKOUT_OPTIONS_INIT;
        reset_options.notify_flags = GIT_CHECKOUT_NOTIFY_UNTRACKED | GIT_CHECKOUT_NOTIFY_IGNORED;
        reset_options.notify_cb = git_reset_callback;
        reset_options.notify_payload = g_git_temporary_dir;
        if(git2.git_oid_fromstrn(&commit_oid, hash.full, GIT_HASH_LEN) == 0 &&
           git2.git_commit_lookup(&commit, repository, &commit_oid) == 0)
        {
            if(git2.git_reset(repository, (git_object *)commit, GIT_RESET_HARD, &reset_options) == 0)
            {
                clone_and_reset_success = 1;
            }
            else
            {
                write_error("git_reset: %s", git2.git_error_last()->message);
            }
            git2.git_commit_free(commit);
        }
        else
        {
            write_error("libgit2: %s", git2.git_error_last()->message);
        }
        git2.git_repository_free(repository);
    }
    else
    {
        write_error("git_clone: %s", git2.git_error_last()->message);
    }

    // NOTE: can only rename after we close the git_repository
    if(clone_and_reset_success)
    {
        platform.delete_directory(dir);
        platform.rename_directory(dir, g_git_temporary_dir);
    }
    success = platform.directory_exists(dir);
    return success;
}

static int
cache_repository(char *dir, size_t size,
                 char *username, char *github_token, char *organization, char *repo, GitCommitHash *req_hash)
{
    int success = 0;
    GitCommitHash hash = req_hash ? *req_hash : retrieve_latest_commit(github_token, organization, repo, 0);
    if(hash_is_valid(&hash))
    {
        format_string(dir, size, "%s/%s_%s", g_cache_dir, repo, hash.full);
        success = platform.directory_exists(dir) || clone_repository(dir, username, github_token, organization, repo, hash);
    }
    return success;
}

static int
push_to_remote(char *dir, char *github_token, char *commit_message, char *username, char *email)
{
    int success = 0;
    git_signature *author;
    git_signature *committer;
    git_repository *repository;
    git_reference *head_ref;
    git_reference *upstream_ref;
    git_index *index;
    git_oid tree_id;
    git_oid commit_id;
    git_tree *tree;
    git_commit *parent_commit;
    git_remote *remote;
    git_buf remote_name = {0};
    if(git2.git_signature_now(&author, username, email) == 0 &&
       git2.git_signature_dup(&committer, author) == 0 &&
       git2.git_repository_open(&repository, dir) == 0 &&
       git2.git_repository_head(&head_ref, repository) == 0 &&
       git2.git_branch_upstream(&upstream_ref, head_ref) == 0 &&
       git2.git_reference_is_branch(head_ref) &&
       git2.git_reference_is_remote(upstream_ref) &&
       git2.git_repository_index(&index, repository) == 0 &&
       git2.git_index_add_all(index, 0, GIT_INDEX_ADD_DEFAULT, 0, 0) == 0 &&
       git2.git_index_write_tree(&tree_id, index) == 0 &&
       git2.git_tree_lookup(&tree, repository, &tree_id) == 0 &&
       git2.git_reference_peel((git_object **)&parent_commit, head_ref, GIT_OBJECT_COMMIT) == 0 &&
       git2.git_commit_create(&commit_id, repository, "HEAD", author, committer, "UTF-8", commit_message,
                              tree, 1, (const git_commit **)&parent_commit) == 0)
    {
        char *upstream_name = (char *)git2.git_reference_name(upstream_ref);
        char *branch_name = (char *)git2.git_reference_name(head_ref);
        if(git2.git_branch_remote_name(&remote_name, repository, upstream_name) == 0 &&
           git2.git_remote_lookup(&remote, repository, remote_name.ptr) == 0)
        {
            char *payload[2];
            payload[0] = username;
            payload[1] = github_token;
            git_push_options push_options = GIT_PUSH_OPTIONS_INIT;
            push_options.callbacks.certificate_check = git_certificate_no_check_callback;
            push_options.callbacks.credentials = git_acquire_credential_callback;
            push_options.callbacks.payload = payload;
            git_strarray refspecs = {0};
            refspecs.count = 1;
            refspecs.strings = &branch_name;
            if(git2.git_remote_push(remote, &refspecs, &push_options) == 0 &&
               git2.git_reset(repository, (git_object *)parent_commit, GIT_RESET_SOFT, 0) == 0)
            {
                success = 1;
            }
            else
            {
                write_error("libgit2: %s", git2.git_error_last()->message);
            }
        }
        else
        {
            write_error("libgit2: %s", git2.git_error_last()->message);
        }
    }
    else
    {
        write_error("libgit2: %s", git2.git_error_last()->message);
    }
    return success;
}
