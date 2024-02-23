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
                if(!CopyFileW(source_child, target_child, 1)) { result = 0; }
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
win32_create_process(char *command, char *workdir)
{
    int result = 0;
    int utf16_command_len1 = MultiByteToWideChar(CP_UTF8, 0, command, -1, 0, 0);
    int utf16_workdir_len1 = MultiByteToWideChar(CP_UTF8, 0, workdir, -1, 0, 0);
    wchar_t *utf16_command = (wchar_t *)allocate_memory(utf16_command_len1 * sizeof(wchar_t));
    wchar_t *utf16_workdir = (wchar_t *)allocate_memory(utf16_workdir_len1 * sizeof(wchar_t));
    if(MultiByteToWideChar(CP_UTF8, 0, command, -1, utf16_command, utf16_command_len1) &&
       MultiByteToWideChar(CP_UTF8, 0, workdir, -1, utf16_workdir, utf16_workdir_len1))
    {
        STARTUPINFOW startup_info = {sizeof(startup_info)};
        PROCESS_INFORMATION process_info;
        if(CreateProcessW(0, utf16_command, 0, 0, 0, 0, 0, utf16_workdir, &startup_info, &process_info))
        {
            result = 1;
        }
    }
    return result;
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
           format_string(global_crt_path, sizeof(global_crt_path), "%s/curl-ca-bundle.crt", root_dir))
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
        git2.git_revwalk_free = (GitRevwalkFree *)GetProcAddress(module, "git_revwalk_free");
        git2.git_treebuilder_free = (GitTreebuilderFree *)GetProcAddress(module, "git_treebuilder_free");
        git2.git_tree_free = (GitTreeFree *)GetProcAddress(module, "git_tree_free");
        git2.git_signature_free = (GitSignatureFree *)GetProcAddress(module, "git_signature_free");
        git2.git_error_last = (GitErrorLast *)GetProcAddress(module, "git_error_last");
        git2.git_blob_rawsize = (GitBlobRawSize *)GetProcAddress(module, "git_blob_rawsize");
        git2.git_tree_entry_type = (GitTreeEntryType *)GetProcAddress(module, "git_tree_entry_type");

        if(git2.git_libgit2_init() > 0)
        {
            success = 1;
        }
        else
        {
            git_error *error = git2.git_error_last();
            write_error("git_libgit2_init error: %s", error->message);
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
    win32_code->calender_time_to_time = win32_calender_time_to_time;
    win32_code->copy_directory = win32_copy_directory;
    win32_code->create_directory = win32_create_directory;
    win32_code->create_process = win32_create_process;
    win32_code->delete_directory = win32_delete_directory;
    win32_code->delete_file = win32_delete_file;
    win32_code->directory_exists = win32_directory_exists;
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