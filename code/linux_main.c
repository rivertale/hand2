#define _GNU_SOURCE 1
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>
typedef int curl_socket_t;
#include "hand.c"
#include "linux_git2.h"
#include "linux_curl.h"

typedef struct LinuxWaitPidData
{
    int exit_code;
    pid_t *process_id;
    pthread_cond_t condition;
    pthread_mutex_t lock;
} LinuxWaitPidData;

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
linux_rename_directory(char *target_path, char *source_path)
{
    int result = 0;
    if(rename(source_path, target_path) == 0)
    {
        result = 1;
    }
    return result;
}

static int
linux_copy_file(char *target_path, char *source_path)
{
    int result = 0;
    FILE *source_handle = fopen(source_path, "rb");
    FILE *target_handle = fopen(target_path, "wb");
    if(source_handle && target_handle)
    {
        fseek(source_handle, 0, SEEK_END);
        size_t file_size = ftell(source_handle);
        fseek(source_handle, 0, SEEK_SET);
        char *content = allocate_memory(file_size);
        if(fread(content, file_size, 1, source_handle) > 0 &&
           fwrite(content, file_size, 1, target_handle) > 0)
        {
            result = 1;
        }
        free_memory(content);
    }
    if(source_handle) { fclose(source_handle); }
    if(target_handle) { fclose(target_handle); }
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
                    linux_copy_file(target_child, source_child);
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
linux_exec_process(ThreadContext *context, int index, char *command, char *work_dir, char *stdout_path, char *stderr_path)
{
    int exit_code = -1;
    pthread_mutex_t *callback_lock = (pthread_mutex_t *)context->callback_lock;
    if(context->on_progress)
    {
        pthread_mutex_lock(callback_lock);
        context->on_progress(index, context->work_count, work_dir);
        pthread_mutex_unlock(callback_lock);
    }

    GrowableBuffer bootstrap = allocate_growable_buffer();
    if(string_len(command) > 0)
    {
        write_growable_buffer(&bootstrap, command, string_len(command));
    }
    null_terminate(&bootstrap);

    GrowableBuffer stdout_content = allocate_growable_buffer();
    GrowableBuffer stderr_content = allocate_growable_buffer();
    int stdout_handle[2];
    int stderr_handle[2];
    int stdout_created = (pipe(stdout_handle) == 0);
    int stderr_created = (pipe(stderr_handle) == 0);
    if(stdout_created && stderr_created)
    {
        pid_t process_id = fork();
        if(process_id == 0) // NOTE: child
        {
            close(stdout_handle[0]);
            close(stderr_handle[0]);
            setpgid(getpid(), getpid());
            dup2(stdout_handle[1], 1);
            dup2(stderr_handle[1], 2);
            close(stdout_handle[1]);
            close(stderr_handle[1]);
            if(chdir(work_dir) == 0)
            {
                execl("/bin/sh", "sh", "-c", bootstrap.memory, (char *)0);
            }
            exit(-1);
        }
        else
        {
            close(stdout_handle[1]);
            close(stderr_handle[1]);
            if(process_id != -1)
            {
                int more_stdout = 1;
                int more_stderr = 1;
                while(more_stdout || more_stderr)
                {
                    fd_set read_set;
                    FD_ZERO(&read_set);
                    FD_SET(stdout_handle[0], &read_set);
                    FD_SET(stderr_handle[0], &read_set);
                    struct timeval timeout = {0};
                    timeout.tv_usec = 100000;
                    if(select(max(stdout_handle[0], stderr_handle[0]) + 1, &read_set, 0, 0, &timeout) == -1) break;

                    int max_byte_read = 4096;
                    if(more_stdout && FD_ISSET(stdout_handle[0], &read_set))
                    {
                        reserve_growable_buffer(&stdout_content, max_byte_read);
                        char *stdout_ptr = stdout_content.memory + stdout_content.used;
                        ssize_t byte_read = read(stdout_handle[0], stdout_ptr, max_byte_read);
                        more_stdout = (byte_read > 0);
                        if(more_stdout) { stdout_content.used += byte_read; }
                        null_terminate(&stdout_content);
                    }
                    if(more_stderr && FD_ISSET(stderr_handle[0], &read_set))
                    {
                        reserve_growable_buffer(&stderr_content, max_byte_read);
                        char *stderr_ptr = stderr_content.memory + stderr_content.used;
                        ssize_t byte_read = read(stderr_handle[0], stderr_ptr, max_byte_read);
                        more_stderr = (byte_read > 0);
                        if(more_stderr) { stderr_content.used += byte_read; }
                        null_terminate(&stderr_content);
                    }
                }

                int status;
                if(waitpid(process_id, &status, 0) != -1 && WIFEXITED(status))
                {
                    exit_code = WEXITSTATUS(status);
                }
                else
                {
                    write_constant_string(&stderr_content, "Unable to obtain exit code");
                }
            }
            else
            {
                write_constant_string(&stderr_content, "Unable to fork");
            }
            close(stdout_handle[0]);
            close(stderr_handle[0]);
        }
    }
    else
    {
        write_constant_string(&stderr_content, "Unable to create pipe");
        if(stdout_created)
        {
            close(stdout_handle[0]);
            close(stdout_handle[1]);
        }
        if(stderr_created)
        {
            close(stderr_handle[0]);
            close(stderr_handle[1]);
        }
    }
    null_terminate(&stdout_content);
    null_terminate(&stderr_content);

    if(stdout_path)
    {
        FILE *file = fopen(stdout_path, "wb");
        if(file)
        {
            fwrite(stdout_content.memory, stdout_content.used, 1, file);
            fclose(file);
        }
    }
    if(stderr_path)
    {
        FILE *file = fopen(stderr_path, "wb");
        if(file)
        {
            fwrite(stderr_content.memory, stderr_content.used, 1, file);
            fclose(file);
        }
    }

    if(context->on_complete)
    {
        pthread_mutex_lock(callback_lock);
        context->on_complete(index, context->work_count, exit_code, work_dir, stdout_content.memory, stderr_content.memory);
        pthread_mutex_unlock(callback_lock);
    }
    free_growable_buffer(&stdout_content);
    free_growable_buffer(&stderr_content);
    return exit_code;
}

static void *
linux_thread_proc(void *param)
{
    ThreadContext *context = (ThreadContext *)param;
    for(int next_to_work = *context->next_to_work;
        next_to_work != context->work_count;
        next_to_work = *context->next_to_work)
    {
        if(__sync_bool_compare_and_swap(context->next_to_work, next_to_work, next_to_work + 1))
        {
            Work *work = context->works + next_to_work;
            char *work_dir = work->work_dir[0] ? work->work_dir : 0;
            char *stdout_path = work->stdout_path[0] ? work->stdout_path : 0;
            char *stderr_path = work->stderr_path[0] ? work->stderr_path : 0;
            work->exit_code = linux_exec_process(context, next_to_work, work->command, work_dir, stdout_path, stderr_path);
        }
    }
    return 0;
}

static void
linux_wait_for_completion(int thread_count, int work_count, Work *works,
                          WorkOnProgressCallback *on_progress, WorkOnCompleteCallback *on_complete)
{
    pthread_mutex_t callback_lock;
    if(pthread_mutex_init(&callback_lock, 0) == 0)
    {
        volatile int next_to_work = 0;
        int *thread_is_valid = (int *)allocate_memory(thread_count * sizeof(*thread_is_valid));
        pthread_t *thread_handles = (pthread_t *)allocate_memory(thread_count * sizeof(*thread_handles));
        ThreadContext *contexts = (ThreadContext *)allocate_memory(thread_count * sizeof(*contexts));
        for(int i = 0; i < thread_count; ++i)
        {
            contexts[i].thread_index = i;
            contexts[i].work_count = work_count;
            contexts[i].works = works;
            contexts[i].next_to_work = &next_to_work;
            contexts[i].on_progress = on_progress;
            contexts[i].on_complete = on_complete;
            contexts[i].callback_lock = &callback_lock;
            thread_is_valid[i] = (pthread_create(&thread_handles[i], 0, linux_thread_proc, &contexts[i]) == 0);
        }

        for(int i = 0; i < thread_count; ++i)
        {
            if(!thread_is_valid[i]) continue;
            pthread_join(thread_handles[i], 0);
        }
        free_memory(contexts);
        free_memory(thread_handles);
        free_memory(thread_is_valid);
    }
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
    git2.git_repository_free = git_repository_free;
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
       format_string(certificate_path, sizeof(certificate_path), "%s/curl-ca-bundle.crt", root_dir) &&
       format_string(global_git_temporary_dir, sizeof(global_git_temporary_dir), "%s/cache/tmp", root_dir))
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
    linux_code->wait_for_completion = linux_wait_for_completion;
    linux_code->calender_time_to_time = linux_calender_time_to_time;
    linux_code->copy_directory = linux_copy_directory;
    linux_code->create_directory = linux_create_directory;
    linux_code->delete_directory = linux_delete_directory;
    linux_code->copy_file = linux_copy_file;
    linux_code->delete_file = linux_delete_file;
    linux_code->directory_exists = linux_directory_exists;
    linux_code->rename_directory = linux_rename_directory;
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