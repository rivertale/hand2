#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
typedef int curl_socket_t;
#include "hand.c"
#include "linux_git2.h"
#include "linux_curl.h"

static time_t
linux_calender_time_to_time(tm *calender_time, int time_zone)
{
    time_t result = 0;
    time_t time_plus_time_zone = timegm(calender_time);
    if(time_plus_time_zone != -1)
    {
        result = time_plus_time_zone - time_zone * 3600;
    }
    return result;
}

static int
linux_directory_exists(char *path)
{
    int result = 0;
    struct stat file_status;
    if(stat(path, &file_status) == 0)
    {
        result = S_ISDIR(file_status.st_mode);
    }
    return result;
}

static int
linux_delete_file(char *path)
{
    int result = (unlink(path) == 0);
    return result;
}

static int
linux_delete_directory(char *dir_path)
{
    int result = 0;
    size_t dir_len = string_len(dir_path);
    DIR *dir_handle = opendir(dir_path);
    if(dir_handle)
    {
        result = 1;
        for(struct dirent *child = readdir(dir_handle);
            child;
            child = readdir(dir_handle))
        {
            char *name = child->d_name;
            if(compare_string(name, ".") || compare_string(name, "..")) continue;

            size_t name_len = string_len(name);
            char *child = allocate_memory(dir_len + 1 + name_len + 1);
            copy_memory(child + 0,                      dir_path, dir_len);
            copy_memory(child + dir_len,                "/",     1);
            copy_memory(child + dir_len + 1,            name,     name_len);
            copy_memory(child + dir_len + 1 + name_len, "\0",    1);

            struct stat file_status;
            if(stat(child, &file_status) != 0)
            {
                result = 0;
            }
            else if(S_ISDIR(file_status.st_mode))
            {
                if(!linux_delete_directory(child)) { result = 0; }
            }
            else
            {
                if(unlink(child) != 0) { result = 0; }
            }
            free_memory(child);
        }
        closedir(dir_handle);
        rmdir(dir_path);
    }
    return result;
}

static int
linux_copy_directory(char *target_dir, char *source_dir)
{
    int result = 0;
    size_t source_len = string_len(source_dir);
    size_t target_len = string_len(target_dir);
    if(mkdir(target_dir, 0777) == 0)
    {
        DIR *dir_handle = opendir(source_dir);
        if(dir_handle)
        {
            result = 1;
            for(struct dirent *child = readdir(dir_handle);
                child;
                child = readdir(dir_handle))
            {
                char *name = child->d_name;
                if(compare_string(name, ".") || compare_string(name, "..")) continue;

                size_t name_len = string_len(name);
                char *source_child = (char *)allocate_memory(source_len + 1 + name_len + 1);
                char *target_child = (char *)allocate_memory(target_len + 1 + name_len + 1);
                copy_memory(source_child + 0,                         source_dir, source_len);
                copy_memory(source_child + source_len,                "/",        1);
                copy_memory(source_child + source_len + 1,            name,       name_len);
                copy_memory(source_child + source_len + 1 + name_len, "\0",       1);
                copy_memory(target_child + 0,                         target_dir, target_len);
                copy_memory(target_child + target_len,                "/",        1);
                copy_memory(target_child + target_len + 1,            name,       name_len);
                copy_memory(target_child + target_len + 1 + name_len, "\0",       1);
                struct stat file_status;
                if(stat(source_child, &file_status) != 0)
                {
                    result = 0;
                }
                else if(S_ISDIR(file_status.st_mode))
                {
                    if(!linux_copy_directory(target_child, source_child)) { result = 0; }
                }
                else
                {
                    FILE *source = fopen(source_child, "rb");
                    FILE *target = fopen(target_child, "wb");
                    if(source && target)
                    {
                        fseek(source, 0, SEEK_END);
                        size_t copy_size = ftell(source);
                        fseek(source, 0, SEEK_SET);
                        char *content = allocate_memory(copy_size);
                        if(fread(content, copy_size, 1, source) == 0 || fwrite(content, copy_size, 1, target) == 0)
                        {
                            result = 0;
                        }
                        free_memory(content);
                    }
                    if(source) fclose(source);
                    if(target) fclose(target);
                }
                free_memory(source_child);
                free_memory(target_child);
            }
            closedir(dir_handle);
        }
    }
    return result;
}

static int
linux_create_directory(char *path)
{
    int result = (mkdir(path, 0777) == 0);
    return result;
}

static int
linux_get_root_dir(char *out_buffer, size_t size)
{
    int result = 0;
    static char full_path[MAX_PATH_LEN];
    ssize_t len = readlink("/proc/self/exe", full_path, MAX_PATH_LEN);
    if(len < MAX_PATH_LEN)
    {
        size_t dir_len = len;
        while(dir_len > 0)
        {
            --dir_len;
            if(full_path[dir_len] == '/') break;
        }
        full_path[dir_len] = 0;

        if(0 < dir_len && dir_len < size)
        {
            result = 1;
            copy_memory(out_buffer, full_path, dir_len + 1);
        }
    }
    return result;
}

static int
linux_create_process(char *command, char *workdir)
{
    int result = 0;
    pid_t process_id = fork();
    if(process_id == 0)
    {
        execl("/bin/sh", "sh", "-c", command, (char *)0);
        exit(0);
    }
    else if(process_id != -1)
    {
        result = 1;
    }
    return result;
}

static int
linux_init_curl(void)
{
    int success = 0;
    curl.curl_easy_cleanup = curl_easy_cleanup;
    curl.curl_easy_init = curl_easy_init;
    curl.curl_easy_setopt = curl_easy_setopt;
    curl.curl_global_cleanup = curl_global_cleanup;
    curl.curl_global_init = curl_global_init;
    curl.curl_multi_add_handle = curl_multi_add_handle;
    curl.curl_multi_cleanup = curl_multi_cleanup;
    curl.curl_multi_info_read = curl_multi_info_read;
    curl.curl_multi_init = curl_multi_init;
    curl.curl_multi_perform = curl_multi_perform;
    curl.curl_multi_poll = curl_multi_poll;
    curl.curl_multi_remove_handle = curl_multi_remove_handle;
    curl.curl_slist_append = curl_slist_append;
    curl.curl_slist_free_all = curl_slist_free_all;

    static char root_dir[MAX_PATH_LEN];
    if(linux_get_root_dir(root_dir, MAX_PATH_LEN) &&
       format_string(global_certificate_path, sizeof(global_certificate_path), "%s/curl-ca-bundle.crt", root_dir))
    {
        CURLcode error = curl_global_init(CURL_GLOBAL_SSL);
        if(error == 0)
        {
            success = 1;
        }
        else
        {
            write_error("curl_global_init error: %d", error);
        }
    }
    return success;
}

static void
linux_cleanup_curl(void)
{
    curl_global_cleanup();
}

static int
linux_init_git(void)
{
    int success = 0;
    git2.git_oid_tostr = git_oid_tostr;
    git2.git_commit_message = git_commit_message;
    git2.git_reference_name = git_reference_name;
    git2.git_blob_rawcontent = git_blob_rawcontent;
    git2.git_commit_id = git_commit_id;
    git2.git_tree_entry_id = git_tree_entry_id;
    git2.git_commit_author = git_commit_author;
    git2.git_commit_committer = git_commit_committer;
    git2.git_blob_create_from_buffer = git_blob_create_from_buffer;
    git2.git_blob_lookup = git_blob_lookup;
    git2.git_branch_create = git_branch_create;
    git2.git_branch_delete = git_branch_delete;
    git2.git_branch_iterator_new = git_branch_iterator_new;
    git2.git_branch_name = git_branch_name;
    git2.git_branch_next = git_branch_next;
    git2.git_branch_set_upstream = git_branch_set_upstream;
    git2.git_branch_upstream = git_branch_upstream;
    git2.git_clone = git_clone;
    git2.git_commit_create = git_commit_create;
    git2.git_commit_lookup = git_commit_lookup;
    git2.git_commit_parent = git_commit_parent;
    git2.git_commit_tree = git_commit_tree;
    git2.git_diff_tree_to_tree = git_diff_tree_to_tree;
    git2.git_libgit2_init = git_libgit2_init;
    git2.git_libgit2_opts = git_libgit2_opts;
    git2.git_libgit2_shutdown = git_libgit2_shutdown;
    git2.git_oid_cmp = git_oid_cmp;
    git2.git_oid_fromstrn = git_oid_fromstrn;
    git2.git_reference_lookup = git_reference_lookup;
    git2.git_reference_name_to_id = git_reference_name_to_id;
    git2.git_remote_create = git_remote_create;
    git2.git_remote_fetch = git_remote_fetch;
    git2.git_remote_lookup = git_remote_lookup;
    git2.git_remote_prune = git_remote_prune;
    git2.git_remote_push = git_remote_push;
    git2.git_repository_open = git_repository_open;
    git2.git_reset = git_reset;
    git2.git_revwalk_hide = git_revwalk_hide;
    git2.git_revwalk_new = git_revwalk_new;
    git2.git_revwalk_next = git_revwalk_next;
    git2.git_revwalk_push = git_revwalk_push;
    git2.git_revwalk_push_head = git_revwalk_push_head;
    git2.git_revwalk_sorting = git_revwalk_sorting;
    git2.git_signature_new = git_signature_new;
    git2.git_treebuilder_new = git_treebuilder_new;
    git2.git_treebuilder_write = git_treebuilder_write;
    git2.git_tree_lookup = git_tree_lookup;
    git2.git_tree_walk = git_tree_walk;
    git2.git_diff_num_deltas = git_diff_num_deltas;
    git2.git_commit_parentcount = git_commit_parentcount;
    git2.git_branch_iterator_free = git_branch_iterator_free;
    git2.git_commit_free = git_commit_free;
    git2.git_diff_free = git_diff_free;
    git2.git_reference_free = git_reference_free;
    git2.git_remote_free = git_remote_free;
    git2.git_revwalk_free = git_revwalk_free;
    git2.git_treebuilder_free = git_treebuilder_free;
    git2.git_tree_free = git_tree_free;
    git2.git_signature_free = git_signature_free;
    git2.git_error_last = git_error_last;
    git2.git_blob_rawsize = git_blob_rawsize;
    git2.git_tree_entry_type = git_tree_entry_type;

    static char root_dir[MAX_PATH_LEN];
    static char certificate_path[MAX_PATH_LEN];
    if(linux_get_root_dir(root_dir, MAX_PATH_LEN) &&
       format_string(certificate_path, sizeof(certificate_path), "%s/curl-ca-bundle.crt", root_dir))
    {
        if(git2.git_libgit2_init() > 0)
        {
            if(git2.git_libgit2_opts(GIT_OPT_SET_SSL_CERT_LOCATIONS, certificate_path, 0) == 0 &&
               git2.git_libgit2_opts(GIT_OPT_SET_OWNER_VALIDATION, 0) == 0)
            {
                success = 1;
            }
            else
            {
                git2.git_libgit2_shutdown();
            }
        }
        else
        {
            const git_error *error = git2.git_error_last();
            write_error("git_libgit2_init error: %s", error->message);
        }
    }
    else
    {
        // TODO: log
    }
    return success;
}

static void
linux_cleanup_git(void)
{
    git2.git_libgit2_shutdown();
}

static void
linux_init_platform(Platform *linux_code)
{
    linux_code->calender_time_to_time = linux_calender_time_to_time;
    linux_code->copy_directory = linux_copy_directory;
    linux_code->create_directory = linux_create_directory;
    linux_code->create_process = linux_create_process;
    linux_code->delete_directory = linux_delete_directory;
    linux_code->delete_file = linux_delete_file;
    linux_code->directory_exists = linux_directory_exists;
    linux_code->get_root_dir = linux_get_root_dir;
}

int
main(int arg_count, char **args)
{
    linux_init_platform(&platform);
    if(!linux_init_curl()) return 0;
    if(!linux_init_git()) return 0;

    run_hand(arg_count - 1, args + 1);
    linux_cleanup_curl();
    linux_cleanup_git();
    return 0;
}