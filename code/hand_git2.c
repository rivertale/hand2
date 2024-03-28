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
git_copy_blob_callback(const char *root, const git_tree_entry *entry, void *payload)
{
    (void)root;

    int result = 0;
    git_repository *source_repo = ((git_repository **)payload)[0];
    git_repository *target_repo = ((git_repository **)payload)[1];
    if(git2.git_tree_entry_type(entry) == GIT_OBJECT_BLOB)
    {
        git_blob *source_blob;
        git_blob *target_blob;
        git_oid source_id = *git2.git_tree_entry_id(entry);
        git_oid target_id;
        if(git2.git_blob_lookup(&source_blob, source_repo, &source_id) == 0 &&
           git2.git_blob_lookup(&target_blob, target_repo, &source_id) != 0)
        {
            result = -1;
            const void *content = git2.git_blob_rawcontent(source_blob);
            git_object_size_t size = git2.git_blob_rawsize(source_blob);
            if(git2.git_blob_create_from_buffer(&target_id, target_repo, content, size) == 0)
            {
                assert(git2.git_oid_cmp(&source_id, &target_id) == 0);
                result = 0;
            }
        }
    }
    return result;
}

static GitCommitHash
get_root_commit(git_repository *repo)
{
    GitCommitHash result = {0};
    git_oid id;
    git_revwalk *walker = 0;
    if(git2.git_revwalk_new(&walker, repo) == 0)
    {
        if(git2.git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_REVERSE) == 0 &&
           git2.git_revwalk_push_head(walker) == 0 &&
           git2.git_revwalk_next(&id, walker) == 0)
        {
            git2.git_oid_tostr(result.full, GIT_HASH_LEN + 1, &id);
        }
        git2.git_revwalk_free(walker);
    }
    return result;
}

static GitCommitHash
find_latest_identical_commit(git_repository *search_repo, GitCommitHash search_hash,
                             git_repository *desired_repo, GitCommitHash desired_hash)
{
    GitCommitHash result = {0};
    git_oid search_id;
    git_oid desired_id;
    git_commit *desired_commit = 0;
    git_tree *desired_tree = 0;
    git_revwalk *walker = 0;
    if(git2.git_oid_fromstrn(&search_id, search_hash.full, GIT_HASH_LEN) == 0 &&
       git2.git_oid_fromstrn(&desired_id, desired_hash.full, GIT_HASH_LEN) == 0 &&
       git2.git_commit_lookup(&desired_commit, desired_repo, &desired_id) == 0 &&
       git2.git_commit_tree(&desired_tree, desired_commit) == 0 &&
       git2.git_revwalk_new(&walker, search_repo) == 0 &&
       git2.git_revwalk_push(walker, &search_id) == 0 &&
       git2.git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL) == 0)
    {
        int found = 0;
        git_oid id;
        while(!found && git2.git_revwalk_next(&id, walker) == 0)
        {
            git_commit *commit = 0;
            git_tree *tree = 0;
            git_diff *diff = 0;
            if(git2.git_commit_lookup(&commit, search_repo, &id) == 0 &&
               git2.git_commit_tree(&tree, commit) == 0 &&
               git2.git_diff_tree_to_tree(&diff, search_repo, desired_tree, tree, 0) == 0 &&
               git2.git_diff_num_deltas(diff) == 0)
            {
                found = 1;
                git2.git_oid_tostr(result.full, GIT_HASH_LEN + 1, git2.git_commit_id(commit));
            }

            if(diff) { git2.git_diff_free(diff); }
            if(tree) { git2.git_tree_free(tree); }
            if(commit) { git2.git_commit_free(commit); }
        }
    }
    else
    {
        // TODO: log
    }

    if(walker) { git2.git_revwalk_free(walker); }
    if(desired_tree) { git2.git_tree_free(desired_tree); }
    if(desired_commit) { git2.git_commit_free(desired_commit); }
    return result;
}

// TODO: rename all req to req
static char *
begin_cache_repository(char *username, char *github_token, char *organization, char *repo, GitCommitHash *req_hash)
{
    char *temporary_dir = 0;
    static char url[MAX_URL_LEN];

    GitCommitHash hash = req_hash ? *req_hash : retrieve_latest_commit(github_token, organization, repo, 0);
    assert(hash_is_valid(&hash));

    if(format_string(g_git_placement_dir, MAX_PATH_LEN, "%s/cache/%s-%s", g_root_dir, repo, hash.full) &&
       format_string(url, MAX_URL_LEN, "https://%s:%s@github.com/%s/%s.git", username, github_token, organization, repo) &&
       !platform.directory_exists(g_git_placement_dir))
    {
        platform.delete_directory(g_git_temporary_dir);
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
                    temporary_dir = g_git_temporary_dir;
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
    }
    return temporary_dir;
}

static void
end_cache_repository(char *dir)
{
    if(dir)
    {
        platform.delete_directory(g_git_placement_dir);
        platform.rename_directory(g_git_placement_dir, g_git_temporary_dir);
    }
}

static int
cache_repository(char *buffer, size_t size, char *username, char *github_token, char *organization, char *repo, GitCommitHash *hash)
{
    char *dir = begin_cache_repository(username, github_token, organization, repo, hash);
    int formatted = format_string(buffer, size, "%s", g_git_placement_dir);
    end_cache_repository(dir);

    int success = formatted && platform.directory_exists(g_git_placement_dir);
    return success;
}

static int
apply_commit_chain(git_repository *target_repo, char *branch, GitCommitHash target_start_hash,
				   git_repository *source_repo, GitCommitHash source_start_hash, GitCommitHash source_end_hash)
{
    int success = 0;
    git_oid source_start_id;
    git_oid source_end_id;
    git_oid target_start_id;
    git_commit *target_start_commit = 0;
    git_reference *target_branch_ref = 0;
    git_remote *remote = 0;
    git_revwalk *walker = 0;
    static char ref[MAX_PATH_LEN];
    static char push_spec[MAX_PATH_LEN * 2];
    if(format_string(ref, MAX_PATH_LEN, "refs/heads/%s", branch) &&
       format_string(push_spec, MAX_PATH_LEN * 2, "refs/heads/%s:refs/heads/%s", branch, branch) &&
       git2.git_oid_fromstrn(&source_start_id, source_start_hash.full, GIT_HASH_LEN) == 0 &&
       git2.git_oid_fromstrn(&source_end_id, source_end_hash.full, GIT_HASH_LEN) == 0 &&
       git2.git_oid_fromstrn(&target_start_id, target_start_hash.full, GIT_HASH_LEN) == 0 &&
       git2.git_commit_lookup(&target_start_commit, target_repo, &target_start_id) == 0 &&
       git2.git_branch_create(&target_branch_ref, target_repo, branch, target_start_commit, 0) == 0 &&
       git2.git_branch_set_upstream(target_branch_ref, branch) == 0 &&
       git2.git_remote_lookup(&remote, target_repo, "origin") == 0 &&
       git2.git_revwalk_new(&walker, source_repo) == 0 &&
       git2.git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_REVERSE) == 0 &&
       git2.git_revwalk_push(walker, &source_end_id) == 0 &&
       git2.git_revwalk_hide(walker, &source_start_id) == 0)
    {
        char *push_spec_ptr = push_spec;
        git_strarray push_specs = {0};
        push_specs.count = 1;
        push_specs.strings = &push_spec_ptr;

        int all_commited = 1;
        git_oid source_id;
        git_oid target_id = target_start_id;
        while(git2.git_revwalk_next(&source_id, walker) == 0)
        {
            git_oid target_tree_id;
            git_commit *source_commit = 0;
            git_commit *target_commit = 0;
            git_tree *source_tree = 0;
            git_tree *target_tree = 0;
            git_treebuilder *tree_builder = 0;
            if(git2.git_commit_lookup(&source_commit, source_repo, &source_id) == 0 &&
               git2.git_commit_lookup(&target_commit, target_repo, &target_id) == 0 &&
               git2.git_commit_tree(&source_tree, source_commit) == 0 &&
               git2.git_treebuilder_new(&tree_builder, target_repo, source_tree) == 0 &&
               git2.git_treebuilder_write(&target_tree_id, tree_builder) == 0 &&
               git2.git_tree_lookup(&target_tree, target_repo, &target_tree_id) == 0)
            {
                git_repository *repo_pair[2] = {0};
                repo_pair[0] = source_repo;
                repo_pair[1] = target_repo;
                const git_signature *author = git2.git_commit_author(source_commit);
                const git_signature *committer = git2.git_commit_committer(source_commit);
                const char *message = git2.git_commit_message(source_commit);
                if(author && committer && message &&
                   git2.git_commit_create(&target_id, target_repo, ref, author, committer,
                                          0, message, target_tree, 1, (const git_commit **)&target_commit) == 0 &&
                   git2.git_tree_walk(source_tree, GIT_TREEWALK_PRE, git_copy_blob_callback, repo_pair) == 0)
                {
                    // NOTE: success
                }
                else
                {
                    // TODO: error
                    all_commited = 0;
                }
            }
            else
            {
                // TODO: log
            }

            if(tree_builder) { git2.git_treebuilder_free(tree_builder); }
            if(target_tree) { git2.git_tree_free(target_tree); }
            if(source_tree) { git2.git_tree_free(source_tree); }
            if(target_commit) { git2.git_commit_free(target_commit); }
            if(source_commit) { git2.git_commit_free(source_commit); }
        }

        if(all_commited)
        {
            if(git2.git_remote_push(remote, &push_specs, 0) == 0)
            {
                // NOTE: success
                success = 1;

            }
            else
            {
                // TODO: error
            }
        }
    }

    if(walker) { git2.git_revwalk_free(walker); }
    if(remote) { git2.git_remote_free(remote); }
    if(target_start_commit) { git2.git_commit_free(target_start_commit); }
    if(target_branch_ref) { git2.git_reference_free(target_branch_ref); }
    return success;
}
