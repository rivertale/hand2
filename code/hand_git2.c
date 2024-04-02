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
    if(format_string(full_path, MAX_PATH_LEN, "%s/%s", parent, path) &&
       platform.delete_file(full_path))
    {
        // NOTE: remove successfully
    }
    else
    {
        //TODO: log
    }
    return 0;
}

static int
cache_repository(char *dir, size_t size,
                 char *username, char *github_token, char *organization, char *repo, GitCommitHash *req_hash)
{
    int success = 0;
    static char url[MAX_URL_LEN];
    GitCommitHash hash = req_hash ? *req_hash : retrieve_latest_commit(github_token, organization, repo, 0);
    assert(hash_is_valid(&hash));

    if(format_string(dir, size, "%s/%s_%s", g_cache_dir, repo, hash.full))
    {
        if(!platform.directory_exists(dir) &&
           format_string(url, MAX_URL_LEN, "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, repo))
        {
            platform.delete_directory(g_git_temporary_dir);

            int clone_and_reset_success = 0;
            git_repository *repository;
            if(git2.git_clone(&repository, url, g_git_temporary_dir, 0) == 0)
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
                    git2.git_commit_free(commit);
                }
                else
                {
                    // TODO: log
                }
                git2.git_repository_free(repository);
            }
            else
            {
                // TODO: log
            }

            // NOTE: can only rename after we close the git_repository that refers to the directory
            if(clone_and_reset_success)
            {
                platform.delete_directory(dir);
                platform.rename_directory(dir, g_git_temporary_dir);
            }
        }
        success = platform.directory_exists(dir);
    }
    return success;
}

static int
push_to_remote(char *dir, char *commit_message, char *username, char *email)
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
            git_strarray refspecs = {0};
            refspecs.count = 1;
            refspecs.strings = &branch_name;
            if(git2.git_remote_push(remote, &refspecs, 0) == 0 &&
               git2.git_reset(repository, (git_object *)parent_commit, GIT_RESET_SOFT, 0) == 0)
            {
                success = 1;
            }
        }
    }
    return success;
}
