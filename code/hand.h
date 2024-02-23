#include <stdlib.h>
#include <assert.h>
#include "cjson/cJSON.c"
#include "hand_common.h"
#include "hand_platform.h"
#include "hand_buffer.h"
#include "hand_curl.h"
#include "hand_git2.h"
#include "hand_config.h"

#define GIT_HASH_LEN 40

typedef struct GitCommitHash
{
    char string[GIT_HASH_LEN + 1];
} GitCommitHash;

static char global_root_dir[MAX_PATH_LEN];

static GitCommitHash
init_git_commit_hash(char *string)
{
    GitCommitHash result = {0};
    if(string_len(string) == GIT_HASH_LEN)
    {
        copy_memory(result.string, string, GIT_HASH_LEN);
    }
    return result;
}

static int
hash_is_valid(GitCommitHash *hash)
{
    int result = 0;
    if(hash)
    {
        result = 1;
        for(int i = 0; i < GIT_HASH_LEN; ++i)
        {
            if(hash->string[i] == 0) { result = 0; }
        }
    }
    return result;
}
