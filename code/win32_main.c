#include <windows.h>
typedef SOCKET curl_socket_t;
#include "hand.c"

static size_t
win32_utf16_len(wchar_t *string)
{
    size_t result = 0;
    while(*string++) { ++result; }
    return result;
}

static int
win32_compare_utf16(wchar_t *a, wchar_t *b)
{
    for(;;)
    {
        if(*a != *b) return 0;
        if(*a == 0) break;
        ++a;
        ++b;
    }
    return 1;
}

static time_t
win32_calender_time_to_time(tm *calender_time, int time_zone)
{
    time_t result = 0;
    time_t time_plus_time_zone = _mkgmtime(calender_time);
    if(time_plus_time_zone != -1)
    {
        result = time_plus_time_zone - time_zone * 3600;
    }
    return result;
}

static int
win32_directory_exists(char *path)
{
    int result = 0;
    static wchar_t wide_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, MAX_PATH_LEN))
    {
        DWORD attrib = GetFileAttributesW(wide_path);
        result = (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
    }
    return result;
}

static int
win32_rename_directory(char *target_path, char *source_path)
{
    int result = 0;
    static wchar_t wide_source_path[MAX_PATH_LEN];
    static wchar_t wide_target_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, source_path, -1, wide_source_path, MAX_PATH_LEN) &&
       MultiByteToWideChar(CP_UTF8, 0, target_path, -1, wide_target_path, MAX_PATH_LEN))
    {
        result = MoveFileW(wide_source_path, wide_target_path);
    }
    return result;
}

static int
win32_copy_file(char *target_path, char *source_path)
{
    int result = 0;
    static wchar_t wide_source_path[MAX_PATH_LEN];
    static wchar_t wide_target_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, source_path, -1, wide_source_path, MAX_PATH_LEN) &&
       MultiByteToWideChar(CP_UTF8, 0, target_path, -1, wide_target_path, MAX_PATH_LEN))
    {
        result = CopyFileW(wide_source_path, wide_target_path, 0);
    }
    return result;
}

static int
win32_delete_file(char *path)
{
    int result = 0;
    static wchar_t wide_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, MAX_PATH_LEN))
    {
        result = DeleteFileW(wide_path);
    }
    return result;
}

static int
win32_delete_directory_by_wide_path(wchar_t *dir_path)
{
    int result = 0;
    static wchar_t wildcard[MAX_PATH_LEN];

    size_t dir_len = win32_utf16_len(dir_path);
    if(dir_len + 2 < MAX_PATH_LEN)
    {
        copy_memory(wildcard + 0,       dir_path, sizeof(wchar_t) * dir_len);
        copy_memory(wildcard + dir_len, L"/*",    sizeof(wchar_t) * 3);

        result = 1;
        WIN32_FIND_DATAW find_data;
        HANDLE find_handle = FindFirstFileW(wildcard, &find_data);
        for(int more_file = (find_handle != INVALID_HANDLE_VALUE);
            more_file;
            more_file = FindNextFileW(find_handle, &find_data))
        {
            wchar_t *name = find_data.cFileName;
            if(win32_compare_utf16(name, L".") || win32_compare_utf16(name, L"..")) continue;

            size_t name_len = win32_utf16_len(name);
            wchar_t *child = (wchar_t *)allocate_memory((dir_len + 1 + name_len + 1) * sizeof(wchar_t));
            copy_memory(child + 0,                      dir_path, sizeof(wchar_t) * dir_len);
            copy_memory(child + dir_len,                L"/",     sizeof(wchar_t));
            copy_memory(child + dir_len + 1,            name,     sizeof(wchar_t) * name_len);
            copy_memory(child + dir_len + 1 + name_len, L"\0",    sizeof(wchar_t));
            if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if(!win32_delete_directory_by_wide_path(child)) { result = 0; }
            }
            else
            {
                if(!DeleteFileW(child)) { result = 0; }
            }
            free_memory(child);
        }

        if(GetLastError() != ERROR_NO_MORE_FILES) { result = 0; }
        if(find_handle != INVALID_HANDLE_VALUE) { FindClose(find_handle); }
        RemoveDirectoryW(dir_path);
    }
    return result;
}

static int
win32_delete_directory(char *path)
{
    int result = 0;
    static wchar_t wide_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, MAX_PATH_LEN))
    {
        result = win32_delete_directory_by_wide_path(wide_path);
    }
    return result;
}

static int
win32_copy_directory_by_wide_path(wchar_t *target_dir, wchar_t *source_dir)
{
    int result = 0;
    static wchar_t wildcard[MAX_PATH_LEN];

    size_t source_len = win32_utf16_len(source_dir);
    size_t target_len = win32_utf16_len(target_dir);
    if(CreateDirectoryW(target_dir, 0) && source_len + 2 < MAX_PATH_LEN)
    {
        copy_memory(wildcard + 0,          source_dir, sizeof(wchar_t) * source_len);
        copy_memory(wildcard + source_len, L"/*",      sizeof(wchar_t) * 3);

        result = 1;
        WIN32_FIND_DATAW find_data;
        HANDLE find_handle = FindFirstFileW(wildcard, &find_data);
        for(int more_file = (find_handle != INVALID_HANDLE_VALUE);
            more_file;
            more_file = FindNextFileW(find_handle, &find_data))
        {
            wchar_t *name = find_data.cFileName;
            if(win32_compare_utf16(name, L".") || win32_compare_utf16(name, L"..")) continue;

            size_t name_len = win32_utf16_len(name);
            wchar_t *source_child = (wchar_t *)allocate_memory((source_len + 1 + name_len + 1) * sizeof(wchar_t));
            wchar_t *target_child = (wchar_t *)allocate_memory((target_len + 1 + name_len + 1) * sizeof(wchar_t));
            copy_memory(source_child + 0,                         source_dir, sizeof(wchar_t) * source_len);
            copy_memory(source_child + source_len,                L"/",       sizeof(wchar_t));
            copy_memory(source_child + source_len + 1,            name,       sizeof(wchar_t) * name_len);
            copy_memory(source_child + source_len + 1 + name_len, L"\0",      sizeof(wchar_t));
            copy_memory(target_child + 0,                         target_dir, sizeof(wchar_t) * target_len);
            copy_memory(target_child + target_len,                L"/",       sizeof(wchar_t));
            copy_memory(target_child + target_len + 1,            name,       sizeof(wchar_t) * name_len);
            copy_memory(target_child + target_len + 1 + name_len, L"\0",      sizeof(wchar_t));
            if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if(!win32_copy_directory_by_wide_path(target_child, source_child)) { result = 0; }
            }
            else
            {
                if(!CopyFileW(source_child, target_child, 0)) { result = 0; }
            }
            free_memory(source_child);
            free_memory(target_child);
        }

        if(GetLastError() != ERROR_NO_MORE_FILES) { result = 0; }
        if(find_handle != INVALID_HANDLE_VALUE) { FindClose(find_handle); }
    }
    return result;
}

static int
win32_copy_directory(char *target_path, char *source_path)
{
    int result = 0;
    static wchar_t wide_target_path[MAX_PATH_LEN];
    static wchar_t wide_source_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, target_path, -1, wide_target_path, MAX_PATH_LEN) &&
       MultiByteToWideChar(CP_UTF8, 0, source_path, -1, wide_source_path, MAX_PATH_LEN))
    {
        result = win32_copy_directory_by_wide_path(wide_target_path, wide_source_path);
    }
    return result;
}

static int
win32_create_directory(char *path)
{
    int result = 0;
    static wchar_t wide_path[MAX_PATH_LEN];
    if(MultiByteToWideChar(CP_UTF8, 0, path, -1, wide_path, MAX_PATH_LEN))
    {
        result = CreateDirectoryW(wide_path, 0);
    }
    return result;
}

static int
win32_get_root_dir(char *out_buffer, size_t size)
{
    int result = 0;
    static wchar_t full_path[MAX_PATH_LEN];
    DWORD len = GetModuleFileNameW(0, full_path, MAX_PATH_LEN);
    if(len < MAX_PATH_LEN)
    {
        size_t result_len = len;
        while(result_len > 0)
        {
            --result_len;
            if(full_path[result_len] == '\\') break;
        }
        full_path[result_len] = 0;

        if(result_len > 0 &&
           WideCharToMultiByte(CP_UTF8, 0, full_path, -1, out_buffer, (int)size, 0, 0))
        {
            result = 1;
        }
    }
    return result;
}

static int
win32_exec_process(ThreadContext *context, int index, char *command, char *work_dir, char *stdout_path, char *stderr_path)
{
    int exit_code = -1;
    HANDLE callback_lock = (HANDLE)context->callback_lock;
    if(context->on_progress)
    {
        WaitForSingleObject(callback_lock, INFINITE);
        context->on_progress(index, context->work_count, work_dir);
        ReleaseMutex(callback_lock);
    }

    int utf16_command_len = MultiByteToWideChar(CP_UTF8, 0, command, -1, 0, 0);
    int utf16_work_dir_len = MultiByteToWideChar(CP_UTF8, 0, work_dir, -1, 0, 0);
    wchar_t *utf16_command = (wchar_t *)allocate_memory(utf16_command_len * sizeof(wchar_t));
    wchar_t *utf16_work_dir = (wchar_t *)allocate_memory(utf16_work_dir_len * sizeof(wchar_t));
    if(MultiByteToWideChar(CP_UTF8, 0, command, -1, utf16_command, utf16_command_len) &&
       MultiByteToWideChar(CP_UTF8, 0, work_dir, -1, utf16_work_dir, utf16_work_dir_len))
    {
        HANDLE stdout_handle = INVALID_HANDLE_VALUE;
        HANDLE stderr_handle = INVALID_HANDLE_VALUE;
        if(global_log_file && stdout_path)
        {
            int utf16_stdout_len = MultiByteToWideChar(CP_UTF8, 0, stdout_path, -1, 0, 0);
            wchar_t *utf16_stdout = (wchar_t *)allocate_memory(utf16_stdout_len * sizeof(wchar_t));
            if(MultiByteToWideChar(CP_UTF8, 0, stdout_path, -1, utf16_stdout, utf16_stdout_len))
            {
                stdout_handle = CreateFileW(utf16_stdout, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
                free_memory(utf16_stdout);
            }
        }
        if(global_log_file && stderr_path)
        {
            int utf16_stderr_len = MultiByteToWideChar(CP_UTF8, 0, stderr_path, -1, 0, 0);
            wchar_t *utf16_stderr = (wchar_t *)allocate_memory(utf16_stderr_len * sizeof(wchar_t));
            if(MultiByteToWideChar(CP_UTF8, 0, stderr_path, -1, utf16_stderr, utf16_stderr_len))
            {
                stderr_handle = CreateFileW(utf16_stderr, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
                free_memory(utf16_stderr);
            }
        }

        STARTUPINFOW startup_info = {0};
        startup_info.cb = sizeof(startup_info);
        startup_info.dwFlags |= STARTF_USESTDHANDLES;
        startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup_info.hStdOutput = (stdout_handle != INVALID_HANDLE_VALUE) ? stdout_handle : GetStdHandle(STD_OUTPUT_HANDLE);
        startup_info.hStdError = (stderr_handle != INVALID_HANDLE_VALUE) ? stderr_handle : GetStdHandle(STD_ERROR_HANDLE);
        PROCESS_INFORMATION process_info;
        if(CreateProcessW(0, utf16_command, 0, 0, 1, 0, 0, utf16_work_dir, &startup_info, &process_info))
        {
            if(stdout_handle != INVALID_HANDLE_VALUE) { CloseHandle(stdout_handle); }
            if(stderr_handle != INVALID_HANDLE_VALUE) { CloseHandle(stderr_handle); }

            DWORD exit_dword;
            if(WaitForSingleObject(process_info.hProcess, INFINITE) == WAIT_OBJECT_0 &&
               GetExitCodeProcess(process_info.hProcess, &exit_dword))
            {
                exit_code = exit_dword;
            }
            CloseHandle(process_info.hProcess);
            CloseHandle(process_info.hThread);
        }
    }

    if(context->on_done)
    {
        WaitForSingleObject(callback_lock, INFINITE);
        context->on_done(index, context->work_count, exit_code, work_dir, stderr_path);
        ReleaseMutex(callback_lock);
    }
    return exit_code;
}

DWORD WINAPI
win32_thread_proc(LPVOID param)
{
    ThreadContext *context = (ThreadContext *)param;
    for(int next_to_work = *context->next_to_work;
        next_to_work != context->work_count;
        next_to_work = *context->next_to_work)
    {
        if(InterlockedCompareExchange((volatile LONG *)context->next_to_work, next_to_work + 1, next_to_work) == next_to_work)
        {
            Work *work = context->works + next_to_work;
            char *work_dir = work->work_dir[0] ? work->work_dir : 0;
            char *stdout_path = work->stdout_path[0] ? work->stdout_path : 0;
            char *stderr_path = work->stderr_path[0] ? work->stderr_path : 0;
            work->exit_code = win32_exec_process(context, next_to_work, work->command, work_dir, stdout_path, stderr_path);
        }
    }
    return 0;
}

static void
win32_wait_for_completion(int thread_count, int work_count, Work *works,
                          WorkOnProgressCallback *on_progress, WorkOnDoneCallback *on_done)
{
    HANDLE callback_lock = CreateMutexA(0, 0, 0);
    if(callback_lock)
    {
        volatile int next_to_work = 0;
        HANDLE *thread_handles = (HANDLE *)allocate_memory(thread_count * sizeof(*thread_handles));
        ThreadContext *contexts = (ThreadContext *)allocate_memory(thread_count * sizeof(*contexts));
        for(int i = 0; i < thread_count; ++i)
        {
            contexts[i].thread_index = i;
            contexts[i].work_count = work_count;
            contexts[i].works = works;
            contexts[i].next_to_work = &next_to_work;
            contexts[i].on_progress = on_progress;
            contexts[i].on_done = on_done;
            contexts[i].callback_lock = (void *)callback_lock;
            thread_handles[i] = CreateThread(0, 1024 * 1024, win32_thread_proc, &contexts[i], 0, 0);
        }

        for(int i = 0; i < thread_count; ++i)
        {
            if(!thread_handles[i]) continue;
            WaitForSingleObject(thread_handles[i], INFINITE);
            CloseHandle(thread_handles[i]);
        }
        free_memory(contexts);
        free_memory(thread_handles);
    }
    else
    {
        // TODO: error
    }
}

static int
win32_init_curl(void)
{
    int success = 0;
    char *dll_path = "libcurl-x64.dll";
    HMODULE module = LoadLibraryA(dll_path);
    if(module)
    {
        curl.curl_easy_cleanup = (CurlEasyCleanup *)GetProcAddress(module, "curl_easy_cleanup");
        curl.curl_easy_init = (CurlEasyInit *)GetProcAddress(module, "curl_easy_init");
        curl.curl_easy_setopt = (CurlEasySetOpt *)GetProcAddress(module, "curl_easy_setopt");
        curl.curl_global_cleanup = (CurlGlobalCleanup *)GetProcAddress(module, "curl_global_cleanup");
        curl.curl_global_init = (CurlGlobalInit *)GetProcAddress(module, "curl_global_init");
        curl.curl_multi_add_handle = (CurlMultiAddHandle *)GetProcAddress(module, "curl_multi_add_handle");
        curl.curl_multi_cleanup = (CurlMultiCleanup *)GetProcAddress(module, "curl_multi_cleanup");
        curl.curl_multi_info_read = (CurlMultiInfoRead *)GetProcAddress(module, "curl_multi_info_read");
        curl.curl_multi_init = (CurlMultiInit *)GetProcAddress(module, "curl_multi_init");
        curl.curl_multi_perform = (CurlMultiPerform *)GetProcAddress(module, "curl_multi_perform");
        curl.curl_multi_poll = (CurlMultiPoll *)GetProcAddress(module, "curl_multi_poll");
        curl.curl_multi_remove_handle = (CurlMultiRemoveHandle *)GetProcAddress(module, "curl_multi_remove_handle");
        curl.curl_slist_append = (CurlSListAppend *)GetProcAddress(module, "curl_slist_append");
        curl.curl_slist_free_all = (CurlSListFreeAll *)GetProcAddress(module, "curl_slist_free_all");

        static char root_dir[MAX_PATH_LEN];
        if(win32_get_root_dir(root_dir, MAX_PATH_LEN) &&
           format_string(global_certificate_path, sizeof(global_certificate_path), "%s/curl-ca-bundle.crt", root_dir))
        {
            CURLcode error = curl.curl_global_init(CURL_GLOBAL_SSL | CURL_GLOBAL_WIN32);
            if(error == 0)
            {
                success = 1;
            }
            else
            {
                write_error("curl_global_init error: %d", error);
            }
        }
    }
    else
    {
        write_error("unable to load '%s'", dll_path);
    }
    return success;
}

static void
win32_cleanup_curl(void)
{
    curl.curl_global_cleanup();
}

static int
win32_init_git(void)
{
    int success = 0;
    char *dll_path = "libgit2-x64.dll";
    HMODULE module = LoadLibraryA(dll_path);
    if(module)
    {
        git2.git_oid_tostr = (GitOidToStr *)GetProcAddress(module, "git_oid_tostr");
        git2.git_commit_message = (GitCommitMessage *)GetProcAddress(module, "git_commit_message");
        git2.git_reference_name = (GitReferenceName *)GetProcAddress(module, "git_reference_name");
        git2.git_blob_rawcontent = (GitBlobRawContent *)GetProcAddress(module, "git_blob_rawcontent");
        git2.git_commit_id = (GitCommitId *)GetProcAddress(module, "git_commit_id");
        git2.git_tree_entry_id = (GitTreeEntryId *)GetProcAddress(module, "git_tree_entry_id");
        git2.git_commit_author = (GitCommitAuthor *)GetProcAddress(module, "git_commit_author");
        git2.git_commit_committer = (GitCommitCommitter *)GetProcAddress(module, "git_commit_committer");
        git2.git_blob_create_from_buffer = (GitBlobCreateFromBuffer *)GetProcAddress(module, "git_blob_create_from_buffer");
        git2.git_blob_lookup = (GitBlobLookup *)GetProcAddress(module, "git_blob_lookup");
        git2.git_branch_create = (GitBranchCreate *)GetProcAddress(module, "git_branch_create");
        git2.git_branch_delete = (GitBranchDelete *)GetProcAddress(module, "git_branch_delete");
        git2.git_branch_iterator_new = (GitBranchIteratorNew *)GetProcAddress(module, "git_branch_iterator_new");
        git2.git_branch_name = (GitBranchName *)GetProcAddress(module, "git_branch_name");
        git2.git_branch_next = (GitBranchNext *)GetProcAddress(module, "git_branch_next");
        git2.git_branch_set_upstream = (GitBranchSetUpstream *)GetProcAddress(module, "git_branch_set_upstream");
        git2.git_branch_upstream = (GitBranchUpstream *)GetProcAddress(module, "git_branch_upstream");
        git2.git_clone = (GitClone *)GetProcAddress(module, "git_clone");
        git2.git_commit_create = (GitCommitCreate *)GetProcAddress(module, "git_commit_create");
        git2.git_commit_lookup = (GitCommitLookup *)GetProcAddress(module, "git_commit_lookup");
        git2.git_commit_parent = (GitCommitParent *)GetProcAddress(module, "git_commit_parent");
        git2.git_commit_tree = (GitCommitTree *)GetProcAddress(module, "git_commit_tree");
        git2.git_diff_tree_to_tree = (GitDiffTreeToTree *)GetProcAddress(module, "git_diff_tree_to_tree");
        git2.git_libgit2_init = (GitLibgit2Init *)GetProcAddress(module, "git_libgit2_init");
        git2.git_libgit2_opts = (GitLibgit2Opts *)GetProcAddress(module, "git_libgit2_opts");
        git2.git_libgit2_shutdown = (GitLibgit2Shutdown *)GetProcAddress(module, "git_libgit2_shutdown");
        git2.git_oid_cmp = (GitOidCmp *)GetProcAddress(module, "git_oid_cmp");
        git2.git_oid_fromstrn = (GitOidFromStrN *)GetProcAddress(module, "git_oid_fromstrn");
        git2.git_reference_lookup = (GitReferenceLookup *)GetProcAddress(module, "git_reference_lookup");
        git2.git_reference_name_to_id = (GitReferenceNameToId *)GetProcAddress(module, "git_reference_name_to_id");
        git2.git_remote_create = (GitRemoteCreate *)GetProcAddress(module, "git_remote_create");
        git2.git_remote_fetch = (GitRemoteFetch *)GetProcAddress(module, "git_remote_fetch");
        git2.git_remote_lookup = (GitRemoteLookup *)GetProcAddress(module, "git_remote_lookup");
        git2.git_remote_prune = (GitRemotePrune *)GetProcAddress(module, "git_remote_prune");
        git2.git_remote_push = (GitRemotePush *)GetProcAddress(module, "git_remote_push");
        git2.git_repository_open = (GitRepositoryOpen *)GetProcAddress(module, "git_repository_open");
        git2.git_reset = (GitReset *)GetProcAddress(module, "git_reset");
        git2.git_revwalk_hide = (GitRevwalkHide *)GetProcAddress(module, "git_revwalk_hide");
        git2.git_revwalk_new = (GitRevwalkNew *)GetProcAddress(module, "git_revwalk_new");
        git2.git_revwalk_next = (GitRevwalkNext *)GetProcAddress(module, "git_revwalk_next");
        git2.git_revwalk_push = (GitRevwalkPush *)GetProcAddress(module, "git_revwalk_push");
        git2.git_revwalk_push_head = (GitRevwalkPushHead *)GetProcAddress(module, "git_revwalk_push_head");
        git2.git_revwalk_sorting = (GitRevwalkSorting *)GetProcAddress(module, "git_revwalk_sorting");
        git2.git_signature_new = (GitSignatureNew *)GetProcAddress(module, "git_signature_new");
        git2.git_treebuilder_new = (GitTreebuilderNew *)GetProcAddress(module, "git_treebuilder_new");
        git2.git_treebuilder_write = (GitTreebuilderWrite *)GetProcAddress(module, "git_treebuilder_write");
        git2.git_tree_lookup = (GitTreeLookup *)GetProcAddress(module, "git_tree_lookup");
        git2.git_tree_walk = (GitTreeWalk *)GetProcAddress(module, "git_tree_walk");
        git2.git_diff_num_deltas = (GitDiffNumDeltas *)GetProcAddress(module, "git_diff_num_deltas");
        git2.git_commit_parentcount = (GitCommitParentCount *)GetProcAddress(module, "git_commit_parentcount");
        git2.git_branch_iterator_free = (GitBranchIteratorFree *)GetProcAddress(module, "git_branch_iterator_free");
        git2.git_commit_free = (GitCommitFree *)GetProcAddress(module, "git_commit_free");
        git2.git_diff_free = (GitDiffFree *)GetProcAddress(module, "git_diff_free");
        git2.git_reference_free = (GitReferenceFree *)GetProcAddress(module, "git_reference_free");
        git2.git_remote_free = (GitRemoteFree *)GetProcAddress(module, "git_remote_free");
        git2.git_repository_free = (GitRepositoryFree *)GetProcAddress(module, "git_repository_free");
        git2.git_revwalk_free = (GitRevwalkFree *)GetProcAddress(module, "git_revwalk_free");
        git2.git_treebuilder_free = (GitTreebuilderFree *)GetProcAddress(module, "git_treebuilder_free");
        git2.git_tree_free = (GitTreeFree *)GetProcAddress(module, "git_tree_free");
        git2.git_signature_free = (GitSignatureFree *)GetProcAddress(module, "git_signature_free");
        git2.git_error_last = (GitErrorLast *)GetProcAddress(module, "git_error_last");
        git2.git_blob_rawsize = (GitBlobRawSize *)GetProcAddress(module, "git_blob_rawsize");
        git2.git_tree_entry_type = (GitTreeEntryType *)GetProcAddress(module, "git_tree_entry_type");

        static char root_dir[MAX_PATH_LEN];
        static char certificate_path[MAX_PATH_LEN];
        if(win32_get_root_dir(root_dir, MAX_PATH_LEN) &&
           format_string(certificate_path, sizeof(certificate_path), "%s/curl-ca-bundle.crt", root_dir) &&
           format_string(global_temporary_clone_dir, sizeof(global_temporary_clone_dir), "%s/cache/tmp", root_dir))
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
    }
    else
    {
        write_error("unable to load '%s'", dll_path);
    }
    return success;
}

static void
win32_cleanup_git(void)
{
    git2.git_libgit2_shutdown();
}

static void
win32_init_platform(Platform *win32_code)
{
    win32_code->wait_for_completion = win32_wait_for_completion;
    win32_code->calender_time_to_time = win32_calender_time_to_time;
    win32_code->copy_directory = win32_copy_directory;
    win32_code->create_directory = win32_create_directory;
    win32_code->delete_directory = win32_delete_directory;
    win32_code->copy_file = win32_copy_file;
    win32_code->delete_file = win32_delete_file;
    win32_code->directory_exists = win32_directory_exists;
    win32_code->rename_directory = win32_rename_directory;
    win32_code->get_root_dir = win32_get_root_dir;
}

int
main(int arg_count, char **args)
{
    win32_init_platform(&platform);
    if(!win32_init_curl()) return 0;
    if(!win32_init_git()) return 0;

    run_hand(arg_count - 1, args + 1);
    win32_cleanup_curl();
    win32_cleanup_git();
    return 0;
}