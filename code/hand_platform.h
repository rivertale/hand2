typedef struct FileIter
{
    char *out_path;
    size_t max_size;
} FileIter;

typedef struct Platform
{
    int (*copy_directory)(char *target_path, char *source_path);
    int (*create_directory)(char *path);
    int (*create_process)(char *command, char *working_directory);
    int (*delete_directory)(char *path);
    int (*delete_file)(char *path);
    int (*directory_exists)(char *path);
    int (*get_root_dir)(char *buffer, size_t size);

    FileIter (*begin_iterate_file)(char *out_path, size_t max_size, char *dir, char *extension);
    int (*file_iter_is_valid)(FileIter *iter);
    void (*file_iter_advance)(FileIter *iter);
    void (*end_iterate_file)(FileIter *iter);
} Platform;

static Platform platform = {0};