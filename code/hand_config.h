#define CONFIG_LOOKAHEAD_MASK 3

enum
{
    Config_github_token         = 0,
    Config_google_token         = 1,
    Config_organization         = 2,
    Config_ta_team              = 3,
    Config_student_team         = 4,
    Config_grade_command        = 5,
    Config_feedback_repo        = 6,
    Config_spreadsheet          = 7,
    Config_key_username         = 8,
    Config_key_student_id       = 9,
    Config_grade_thread_count   = 10,
    Config_penalty_per_day      = 11,
    Config_score_relative_path  = 12,
    Config_one_past_last,
};

static char *global_config_key_name[] =
{
    "github_access_token",
    "google_api_key",
    "organization",
    "ta_team",
    "student_team",
    "grade_command",
    "feedback_repository",
    "spreadsheet",
    "sheet_key_username",
    "sheet_key_student_id",
    "grade_thread_count",
    "penalty_per_day",
    "score_relative_path",
};

enum
{
    Token_eof,
    Token_identifier,
    Token_equal,
    Token_string,
};

typedef struct Config
{
    char *value[Config_one_past_last];
} Config;

typedef struct Token
{
    int type;
    int len;
    char *identifier;
} Token;

typedef struct ConfigParser
{
    size_t at, content_size;
    int lookahead_to_read, lookahead_to_write;
    Token lookaheads[CONFIG_LOOKAHEAD_MASK + 1];

    int has_error;
    int padding_;
    char *content;
} ConfigParser;

typedef struct ArgParser
{
    int at;
    int arg_count;
    char **args;
} ArgParser;
