#define MAX_PATH_LEN 65536
#define TIME_ZONE_UTC0 (+0)
#define TIME_ZONE_UTC8 (+8)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

static FILE *g_log_file = {0};

static size_t
next_pow_of_two(size_t value)
{
    --value;
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value |= (value >> 16);
    value |= (value >> 32);
    return value + 1;
}

static int
true_for_all(int *booleans, int count)
{
    for(int i = 0; i < count; ++i)
    {
        if(!booleans[i]) return 0;
    }
    return 1;
}

static size_t
string_len(char *string)
{
    size_t result = 0;
    while(*string++) { ++result; }
    return result;
}

static void
clear_memory(void *ptr, size_t size)
{
    char *byte_ptr = (char *)ptr;
    while(size-- > 0) { *byte_ptr++ = 0; }
}

static void
copy_memory(void *dest, void *source, size_t size)
{
    char *byte_source = (char *)source;
    char *byte_dest = (char *)dest;
    while(size-- > 0) { *byte_dest++ = *byte_source++; }
}

static int
compare_substring(char *a, char *b, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        if(a[i] != b[i]) return 0;
        if(a[i] == 0) return 0;
        if(b[i] == 0) return 0;
    }
    return 1;
}

static int
compare_string(char *a, char *b)
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

static int
compare_case_insensitive(char *a, char *b)
{
    for(;;)
    {
        char c_a = *a;
        char c_b = *b;
        if('A' <= c_a && c_a <= 'Z') { c_a = c_a - 'A' + 'a'; }
        if('A' <= c_b && c_b <= 'Z') { c_b = c_b - 'A' + 'a'; }
        if(c_a != c_b) return 0;
        if(c_a == 0) break;
        ++a;
        ++b;
    }
    return 1;
}

static tm
calendar_time(time_t time)
{
    tm result = {0};
    time_t local_time = time + TIME_ZONE_UTC8 * 3600;
    tm *local_tm = gmtime(&local_time);
    if(local_tm) { result = *local_tm; }
    return result;
}

static tm
current_calendar_time(void)
{
    time_t now = time(0);
    return calendar_time(now);
}

static void
write_log_with_args(char *format, va_list arg_list)
{
    if(g_log_file)
    {
        vfprintf(g_log_file, format, arg_list);
        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    }
}

static void
write_log(char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    write_log_with_args(format, arg_list);
    va_end(arg_list);
}

static void
write_output(char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    vprintf(format, arg_list);
    printf("\n");
    fflush(stdout);
    va_end(arg_list);
}

static void
write_error(char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    printf("[ERROR] ");
    vprintf(format, arg_list);
    printf("\n");
    fflush(stdout);
    write_log_with_args(format, arg_list);
    va_end(arg_list);
}

static char *
allocate_memory(size_t size)
{
    char *result = (char *)malloc(size);
    if(!result)
    {
        write_error("fatal: out of memory");
        exit(0);
    }
    return result;
}

static void
free_memory(void *ptr)
{
    if(ptr) { free(ptr); }
}

static int
format_string(char *buffer, size_t size, char *format, ...)
{
    int result = 0;

    va_list arg_list;
    va_start(arg_list, format);
    int required_size = vsnprintf(buffer, size, format, arg_list);
    if(required_size < 0)
    {
        if(size >= 1) { *buffer = 0; }
        write_log("vsnprintf error: %d", required_size);
    }
    else if((size_t)required_size >= size)
    {
        write_log("buffer too short: %s", buffer);
    }
    else
    {
        result = 1;
    }
    return result;
}
