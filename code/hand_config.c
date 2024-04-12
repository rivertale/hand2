static char
peek_char(ConfigParser *parser)
{
    char result = 0;
    if(parser->at < parser->content_size)
        result = parser->content[parser->at];
    return result;
}

static void
eat_char(ConfigParser *parser)
{
    if(parser->at < parser->content_size)
        ++parser->at;
}

static char
eat_and_peek_char(ConfigParser *parser)
{
    eat_char(parser);
    return peek_char(parser);
}

static void
eat_whitespaces(ConfigParser *parser)
{
    int in_comment = 0;
    for(char c = peek_char(parser); c; c = eat_and_peek_char(parser))
    {
        if(in_comment)
        {
            if(c == '\n')
                in_comment = 0;
        }
        else
        {
            if(c == '#')
                in_comment = 1;
            else if(c != ' ' && c != '\t' && c != '\r' && c != '\n')
                break;
        }
    }
}

static Token
lex_token(ConfigParser *parser)
{
    Token token = {0};
    eat_whitespaces(parser);

    char c0 = peek_char(parser);
    if(c0 == '\0')
    {
        token.type = Token_eof;
    }
    else if(c0 == '=')
    {
        eat_char(parser);
        token.type = Token_equal;
    }
    else if(c0 == '"' || c0 == '\'')
    {
        eat_char(parser);
        token.type = Token_string;
        token.identifier = parser->content + parser->at;
        for(char c = peek_char(parser); c; c = eat_and_peek_char(parser))
        {
            if(c == c0)
                break;

            ++token.len;
        }

        if(peek_char(parser) == '\0')
        {
            char string[256];
            int string_len = token.len < 255 ? token.len : 255;
            copy_memory(string, token.identifier, string_len);
            string[string_len] = 0;
            write_error("Config: string not terminated: '%s'", string);
        }
        else
        {
            eat_char(parser);
        }
    }
    else if(('A' <= c0 && c0 <= 'Z') || ('a' <= c0 && c0 <= 'z'))
    {
        token.type = Token_identifier;
        token.identifier = parser->content + parser->at;
        for(char c = peek_char(parser); c; c = eat_and_peek_char(parser))
        {
            if(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '_' || c == '.')
                ++token.len;
            else
                break;
        }
    }
    else
    {
        eat_char(parser);
        write_error("Config: unrecognized char '%c'", c0);
    }
    return token;
}

static Token
peek_token(ConfigParser *parser, int lookahead)
{
    assert(lookahead <= CONFIG_LOOKAHEAD_MASK);
    for(int offset = 0; offset <= lookahead; ++offset)
    {
        int index = (parser->lookahead_to_read + offset) & CONFIG_LOOKAHEAD_MASK;
        if(index == parser->lookahead_to_write)
        {
            parser->lookaheads[index] = lex_token(parser);
            parser->lookahead_to_write = (parser->lookahead_to_write + 1) & CONFIG_LOOKAHEAD_MASK;
        }
    }
    return parser->lookaheads[(parser->lookahead_to_read + lookahead) & CONFIG_LOOKAHEAD_MASK];
}

static void
eat_token(ConfigParser *parser)
{
    if(parser->lookahead_to_read == parser->lookahead_to_write)
    {
        peek_token(parser, 0);
    }
    parser->lookahead_to_read = (parser->lookahead_to_read + 1) & CONFIG_LOOKAHEAD_MASK;
}

static int
parse_key_value_pair(ConfigParser *parser,
                     char **out_key, char **out_value, int *out_key_len, int *out_value_len)
{
    int more_to_parse = 0;
    for(;;)
    {
        Token t0 = peek_token(parser, 0);
        Token t1 = peek_token(parser, 1);
        Token t2 = peek_token(parser, 2);
        if(t0.type == Token_eof)
            break;

        if(t0.type == Token_identifier && t1.type == Token_equal && t2.type == Token_string)
        {
            more_to_parse = 1;
            *out_key = t0.identifier;
            *out_key_len = t0.len;
            *out_value = t2.identifier;
            *out_value_len = t2.len;
            eat_token(parser);
            eat_token(parser);
            eat_token(parser);
            break;
        }
        else
        {
            if(t0.type == Token_identifier)
                eat_token(parser);
            if(t0.type == Token_identifier && t1.type == Token_equal)
                eat_token(parser);

            Token t = peek_token(parser, 0);
            static char token_content[256];
            size_t token_len = min(t.len, sizeof(token_content) - 1);
            copy_memory(token_content, t.identifier, token_len);
            copy_memory(token_content + token_len, "\0", 1);
            write_error("Unexpected token: '%s'", token_content);
            eat_token(parser);
        }
    }
    return more_to_parse;
}

static char *
allocate_and_copy_string(char *string, size_t size)
{
    char *result = allocate_memory(size + 1);
    copy_memory(result, string, size);
    result[size] = 0;
    return result;
}

static ConfigParser
init_config_parser(char *content, size_t size)
{
    ConfigParser result = {0};
    result.content = content;
    result.content_size = size;
    return result;
}

static void
free_config(Config *config)
{
    for(int i = 0; i < Config_one_past_last; ++i)
    {
        free_memory(config->value[i]);
    }
    clear_memory(config, sizeof(config));
}

static int
load_config(Config *config, char *path)
{
    clear_memory(config, sizeof(*config));

    int success = 1;
    FILE *file = platform.fopen(path, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *file_content = allocate_memory(file_size);
        if(fread(file_content, file_size, 1, file))
        {
            ConfigParser parser = init_config_parser(file_content, file_size);
            for(;;)
            {
                int key_len, value_len;
                char *key, *value;
                if(!parse_key_value_pair(&parser, &key, &value, &key_len, &value_len))
                    break;

                int key_found = 0;
                for(int i = 0; i < Config_one_past_last; ++i)
                {
                    if(compare_substring(key, g_config_name[i], key_len))
                    {
                        key_found = 1;
                        config->value[i] = allocate_and_copy_string(value, value_len);
                        break;
                    }
                }

                if(!key_found)
                {
                    char unrecognized_key[256];
                    int out_key_len = min(key_len, 255);
                    copy_memory(unrecognized_key, key, out_key_len);
                    unrecognized_key[out_key_len] = 0;
                    write_error("Unrecognized config '%s'", unrecognized_key);
                }
            }
        }
        else
        {
            write_error("Read file '%s' failed", path);
        }
        free_memory(file_content);
        fclose(file);
    }

    for(int i = 0; i < Config_one_past_last; ++i)
    {
        if(!config->value[i])
        {
            success = 0;
            write_error("Config: incomplete config because key '%s' not found", g_config_name[i]);
        }
    }
    return success;
}

static int
ensure_config_exists(char *path)
{
    int config_exists = 0;

    FILE *file = platform.fopen(path, "rb");
    if(file)
    {
        config_exists = 1;
        fclose(file);
    }
    else
    {
        file = platform.fopen(path, "wb");
        if(file)
        {
            size_t size = sizeof(g_default_config) - 1;
            fwrite(g_default_config, size, 1, file);
            fclose(file);
        }
    }
    return config_exists;
}

static char *
next_arg(ArgParser *parser)
{
    char *result = 0;
    if(parser->at < parser->arg_count)
    {
        result = parser->args[parser->at++];
    }
    return result;
}

static char *
next_option(ArgParser *parser)
{
    char *result = 0;
    if(parser->at < parser->arg_count)
    {
        char *arg = parser->args[parser->at];
        if(arg[0] == '-')
        {
            result = arg;
            ++parser->at;
        }
    }
    return result;
}

static ArgParser
init_arg_parser(int arg_count, char **args)
{
    ArgParser result = {0};
    result.arg_count = arg_count;
    result.args = args;
    return result;
}
