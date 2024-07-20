#define CONFIG_LOOKAHEAD_MASK 3

enum
{
    Config_github_token = 0,
    Config_google_token = 1,
    Config_organization = 2,
    Config_ta_team = 3,
    Config_student_team = 4,
    Config_feedback_repo = 5,
    Config_spreadsheet = 6,
    Config_username_label = 7,
    Config_student_id_label = 8,
    Config_email = 9,
    Config_grade_command = 10,
    Config_grade_thread_count = 11,
    Config_score_relative_path = 12,
    Config_penalty_per_day = 13,
    Config_weekends_as_one_day = 14,
    Config_one_past_last,
};

static char *g_config_name[Config_one_past_last] =
{
    "github_access_token",
    "google_api_key",
    "organization",
    "ta_team",
    "student_team",
    "feedback_repository",
    "spreadsheet",
    "username_label",
    "student_id_label",
    "email",
    "grade_command",
    "grade_thread",
    "score_relative_path",
    "penalty_per_day",
    "weekends_as_one_day",
};

static char g_default_config[] =
    "# See how to create a GitHub classic personal access token at:"                                                            "\n"
    "# https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens#creating-a-personal-access-token-classic" "\n"
    "github_access_token = \"your-github-personal-access-token\""                                                               "\n"
                                                                                                                                "\n"
    "# See how to create a Google API key at:"                                                                                  "\n"
    "# https://support.google.com/googleapi/answer/6158862"                                                                     "\n"
    "google_api_key = \"your-google-api-key\""                                                                                  "\n"
                                                                                                                                "\n"
    "# The course's GitHub organization"                                                                                        "\n"
    "# (https://github.com/<github-organization>)"                                                                              "\n"
    "organization = \"github-organization\""                                                                                    "\n"
                                                                                                                                "\n"
    "# TA team slug in the GitHub organization"                                                                                 "\n"
    "# (https://github.com/orgs/<github-organization>/teams/<ta-team-slug>"                                                     "\n"
    "ta_team = \"ta-team-slug\""                                                                                                "\n"
                                                                                                                                "\n"
    "# Student team slug in the GitHub organization"                                                                            "\n"
    "# (https://github.com/orgs/<github-organization>/teams/<student-team-slug>"                                                "\n"
    "student_team = \"student-team-slug\""                                                                                      "\n"
                                                                                                                                "\n"
    "# Repository that stores feedbacks for students before announcing grades"                                                  "\n"
    "# (https://github.com/<github-organization>/<hw-feedback>)"                                                                "\n"
    "feedback_repository = \"hw-feedback\""                                                                                     "\n"
                                                                                                                                "\n"
    "# Google spreadsheet for organizing students' grades, it must be visible to the public"                                    "\n"
    "# (https://docs.google.com/spreadsheets/d/<spreadsheet-id>)"                                                               "\n"
    "spreadsheet = \"spreadsheet-id\""                                                                                          "\n"
                                                                                                                                "\n"
    "# Label corresponding to GitHub username (labels are the first row in the homework sheet)"                                 "\n"
    "username_label = \"github_handle\""                                                                                        "\n"
                                                                                                                                "\n"
    "# Label corresponding to student id (labels are the first row in the homework sheet)"                                      "\n"
    "student_id_label = \"student_id\""                                                                                         "\n"
                                                                                                                                "\n"
    "# An email linked to your GitHub account, it's used to commit feedbacks to <feedback_repository> when grading homework"    "\n"
    "email = \"your@github.email\""                                                                                             "\n"
                                                                                                                                "\n"
    "# Command executed under the homework directory for grading"                                                               "\n"
    "grade_command = 'make activate ARGS=\"make autograde\"'"                                                                   "\n"
                                                                                                                                "\n"
    "# Number of threads used for parallel grading"                                                                             "\n"
    "grade_thread = \"8\""                                                                                                      "\n"
                                                                                                                                "\n"
    "# Relative path to the score file, the score file is generated by <grade_command>"                                         "\n"
    "score_relative_path = \"./test/result/score.txt\""                                                                         "\n"
                                                                                                                                "\n"
    "# Penalty received per day for late submission (in percentage)"                                                            "\n"
    "penalty_per_day = \"15\""                                                                                                  "\n"
                                                                                                                                "\n"
    "# If not 0, weekends are considered as one day when calculating penalty"                                                   "\n"
    "weekends_as_one_day = \"1\""                                                                                               "\n";

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
