#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
typedef struct tm tm;

#include "cjson/cJSON.c"
#include "hand_common.h"
#include "hand_platform.h"
#include "hand_buffer.h"
#include "hand_curl.h"
#include "hand_git2.h"
#include "hand_config.h"

#define GIT_HASH_LEN 40
#define GIT_SHORT_HASH_LEN 7

typedef struct GitCommitHash
{
    char full[GIT_HASH_LEN + 1];
    char trim[GIT_SHORT_HASH_LEN + 1];
} GitCommitHash;

static char g_root_dir[MAX_PATH_LEN];
static char g_cache_dir[MAX_PATH_LEN];
static char g_log_dir[MAX_PATH_LEN];

static GitCommitHash
init_git_commit_hash(char *string)
{
    GitCommitHash result = {0};
    if(string_len(string) == GIT_HASH_LEN)
    {
        copy_memory(result.full, string, GIT_HASH_LEN);
        copy_memory(result.trim, string, GIT_SHORT_HASH_LEN);
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
            if(hash->full[i] == 0)
                result = 0;
        }
    }
    return result;
}

static int
compare_hash(GitCommitHash *a, GitCommitHash *b)
{
    int result = 0;
    if(a && b)
        result = compare_string(a->full, b->full);
    return result;
}

static time_t
parse_time(char *string, int time_zone)
{
    // NOTE: only accept YYYY-MM-DD or YYYY-MM-DD-hh-mm-ss
    time_t result = 0;
    size_t len = string_len(string);
    if(len == 10 &&
       is_digit(string[0]) && is_digit(string[1]) && is_digit(string[2]) && is_digit(string[3]) &&
       is_digit(string[5]) && is_digit(string[6]) && is_digit(string[8]) && is_digit(string[9]) &&
       !is_digit(string[4]) && !is_digit(string[7]))
    {
        // NOTE: YYYY-MM-DD
        tm t = {0};
        t.tm_mday = (string[8] - '0') * 10 + (string[9] - '0');
        t.tm_mon  = (string[5] - '0') * 10 + (string[6] - '0') - 1;
        t.tm_year = (string[0] - '0') * 1000 + (string[1] - '0') * 100 + (string[2] - '0') * 10 + (string[3] - '0') - 1900;
        if(0 <= t.tm_mon && t.tm_mon <= 11 && 1 <= t.tm_mday && t.tm_mday <= 31)
            result = platform.calender_time_to_time(&t, time_zone);
    }
    else if((len == 19 || len == 20) &&
            is_digit(string[0]) && is_digit(string[1]) && is_digit(string[2]) && is_digit(string[3]) &&
            is_digit(string[5]) && is_digit(string[6]) && is_digit(string[8]) && is_digit(string[9]) &&
            is_digit(string[11]) && is_digit(string[12]) && is_digit(string[14]) && is_digit(string[15]) &&
            is_digit(string[17]) && is_digit(string[18]) &&
            !is_digit(string[4]) && !is_digit(string[7]) && !is_digit(string[10]) && !is_digit(string[13]) &&
            !is_digit(string[16]) && !is_digit(string[19]))
    {
        // NOTE: YYYY-MM-DD-hh-mm-ss or YYYY-MM-DDThh:mm:ssZ
        tm t = {0};
        t.tm_sec  = (string[17] - '0') * 10 + (string[18] - '0');
        t.tm_min  = (string[14] - '0') * 10 + (string[15] - '0');
        t.tm_hour = (string[11] - '0') * 10 + (string[12] - '0');
        t.tm_mday = (string[8] - '0') * 10 + (string[9] - '0');
        t.tm_mon  = (string[5] - '0') * 10 + (string[6] - '0') - 1;
        t.tm_year = (string[0] - '0') * 1000 + (string[1] - '0') * 100 + (string[2] - '0') * 10 + (string[3] - '0') - 1900;
        if(0 <= t.tm_mon && t.tm_mon <= 11 && 1 <= t.tm_mday && t.tm_mday <= 31 &&
           0 <= t.tm_hour && t.tm_hour <= 23 && 0 <= t.tm_min && t.tm_min <= 59 && 0 <= t.tm_sec && t.tm_sec <= 59)
            result = platform.calender_time_to_time(&t, time_zone);
    }
    return result;
}
