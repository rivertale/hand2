typedef void WorkOnProgressCallback(int index, int count, char *work_dir);
typedef void WorkOnCompleteCallback(int index, int count, int exit_code, char *work_dir, char *stdout_content, char *stderr_content);

typedef struct Work
{
    int exit_code;
    char command[MAX_PATH_LEN];
    char work_dir[MAX_PATH_LEN];
    char stdout_path[MAX_PATH_LEN];
    char stderr_path[MAX_PATH_LEN];
} Work;

typedef struct ThreadContext
{
    int thread_index;
    int work_count;
    Work *works;
    volatile int *next_to_work;
    void *callback_lock;
    WorkOnProgressCallback *on_progress;
    WorkOnCompleteCallback *on_complete;
} ThreadContext;

typedef struct Platform
{
    time_t (*calender_time_to_time)(tm *calender_time, int time_zone);

    // IMPORTANT: there is no timeout on grading since there isn't an obvious way to force terminate a
    // container without using "docker stop <name or container_id>", which requires another config to state
    // how to stop the grade process on timeout (in case we use other grading process in the future), and
    // also requires a way to convey the container name or id.
    void (*wait_for_completion)(int thread_count, int work_count, Work *works,
                                WorkOnProgressCallback *on_progress, WorkOnCompleteCallback *on_complete);

    int (*copy_directory)(char *target_path, char *source_path);
    int (*rename_directory)(char *target_path, char *source_path);
    int (*copy_file)(char *target_path, char *source_path);
    int (*create_directory)(char *path);
    int (*delete_directory)(char *path);
    int (*delete_file)(char *path);
    int (*directory_exists)(char *path);
    void (*sleep)(unsigned int milliseconds);
    FILE *(*fopen)(char *path, char *mode);
} Platform;

static Platform platform = {0};
