#define CONFIG_LOOKAHEAD_MASK 3

enum
{
    Config_email                = 0,
    Config_github_token         = 1,
    Config_google_token         = 2,
    Config_organization         = 3,
    Config_ta_team              = 4,
    Config_student_team         = 5,
    Config_grade_command        = 6,
    Config_feedback_repo        = 7,
    Config_spreadsheet          = 8,
    Config_username_label       = 9,
    Config_student_id_label     = 10,
    Config_grade_thread_count   = 11,
    Config_penalty_per_day      = 12,
    Config_score_relative_path  = 13,
    Config_one_past_last,
};

static char *g_config_name[] =
{
    "email",
    "github_access_token",
    "google_api_key",
    "organization",
    "ta_team",
    "student_team",
    "grade_command",
    "feedback_repository",
    "spreadsheet",
    "username_label",
    "student_id_label",
    "grade_thread_count",
    "penalty_per_day",
    "score_relative_path",
};

static char g_default_config[] =
    "# Email use for commit"                                            "\n"
    "email = \"your@commit.email\""                                     "\n"
                                                                        "\n"
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
    "# sheet_label_username"                                            "\n"
    "username_label = \"label-for-username-in-sheet\""                  "\n"
                                                                        "\n"
    "# sheet_label_student_id"                                          "\n"
    "student_id_label = \"label-for-student-id-in-sheet\""              "\n"
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
