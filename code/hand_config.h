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

static char *g_config_key_name[] =
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
static char g_default_config[] =
    "# The course's GitHub organization name"                           "\n"
    "organization = \"github-organization-name\""                       "\n"
                                                                        "\n"
    "# https://github.com/blog/1509-personal-api-tokens"                "\n"
    "github_access_token = \"your-github-personal-api-token\""          "\n"
                                                                        "\n"
    "# https://cloud.google.com/docs/authentication/api-keys"           "\n"
    "google_api_key = \"your-google-api_key\""                          "\n"
                                                                        "\n"
    "# The student team name in the GitHub organization"                "\n"
    "student_team = \"student-team-name\""                              "\n"
                                                                        "\n"
    "# The TA team name in the GitHub organization"                     "\n"
    "ta_team = \"ta-team-name\""                                        "\n"
                                                                        "\n"
    "# The command executed under homework directory for grading"       "\n"
    "grade_command = \"executed command for grading\""                  "\n"
                                                                        "\n"
    "# feedback_repository"                                             "\n"
    "feedback_repository = \"feedbacks\""                               "\n"
                                                                        "\n"
    "# spreadsheet"                                                     "\n"
    "spreadsheet = \"spreadsheet-id\""                                  "\n"
                                                                        "\n"
    "# sheet_key_username"                                              "\n"
    "sheet_key_username = \"key-for-username-in-sheet\""                "\n"
                                                                        "\n"
    "# sheet_key_student_id"                                            "\n"
    "sheet_key_student_id = \"key-for-student-id-in-sheet\""            "\n"
                                                                        "\n"
    "# penalty per day"                                                 "\n"
    "penalty_per_day = \"15\""                                          "\n"
                                                                        "\n"
    "# relative path for 'score.txt'"                                   "\n"
    "score_relative_path = \"./test/result/score.txt\""                 "\n"
                                                                        "\n"
    "# thread count for grading"                                        "\n"
    "grade_thread_count = \"1\""                                        "\n";

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
