#define write_constant_string(buffer, array) write_growable_buffer(buffer, array, sizeof(array) - 1)

static void
free_growable_buffer(GrowableBuffer *buffer)
{
    free_memory(buffer->memory);
    clear_memory(buffer, sizeof(*buffer));
}

static void
reserve_growable_buffer(GrowableBuffer *buffer, size_t size)
{
    size_t total_size = buffer->used + size + 1;
    if(total_size > buffer->max)
    {
        size_t allocated_size = next_pow_of_two(total_size);
        char *memory = allocate_memory(allocated_size);
        copy_memory(memory, buffer->memory, buffer->used);
        memory[buffer->used] = 0;
        free_memory(buffer->memory);
        buffer->max = allocated_size;
        buffer->memory = memory;
    }
}

static void
write_growable_buffer(GrowableBuffer *buffer, void *data, size_t size)
{
    reserve_growable_buffer(buffer, size);
    copy_memory(buffer->memory + buffer->used, data, size);
    buffer->used += size;
    buffer->memory[buffer->used] = 0;
}

static void
null_terminate(GrowableBuffer *buffer)
{
    reserve_growable_buffer(buffer, 0);
    buffer->memory[buffer->used] = 0;
}

static void
clear_growable_buffer(GrowableBuffer *buffer)
{
    buffer->used = 0;
    null_terminate(buffer);
}

static GrowableBuffer
allocate_growable_buffer(void)
{
    size_t initial_size = 4096;
    GrowableBuffer result = {0};
    result.max = initial_size;
    result.memory = allocate_memory(initial_size);
    null_terminate(&result);
    return result;
}

static GrowableBuffer
read_entire_file(char *path)
{
    GrowableBuffer result = allocate_growable_buffer();
    FILE *file = platform.fopen(path, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = (size_t)ftell(file);
        fseek(file, 0, SEEK_SET);
        reserve_growable_buffer(&result, file_size);
        fread(result.memory + result.used, file_size, 1, file);
        result.used += file_size;
        fclose(file);
    }
    null_terminate(&result);
    return result;
}

static void
free_string_array(StringArray *array)
{
    free_growable_buffer(&array->buffer);
    free_memory(array->elem);
    clear_memory(array, sizeof(*array));
}

static void
reserve_string_array(StringArray *array, int count)
{
    int total = array->count + count;
    if(total > array->max)
    {
        int allocated = (int)next_pow_of_two(total);
        char **elem = (char **)allocate_memory(allocated * sizeof(*elem));
        copy_memory(elem, array->elem, array->count * sizeof(*elem));
        free_memory(array->elem);
        array->max = allocated;
        array->elem = elem;
    }
}

static StringArray
allocate_string_array(void)
{
    int initial_count = 128;
    StringArray result = {0};
    result.max = initial_count;
    result.elem = (char **)allocate_memory(initial_count * sizeof(*result.elem));
    result.buffer = allocate_growable_buffer();
    return result;
}

static void
append_string_array(StringArray *array, char *string)
{
    GrowableBuffer *buffer = &array->buffer;
    size_t old_size = buffer->used;
    char *old_ptr = buffer->memory;

    write_growable_buffer(buffer, string, string_len(string));
    write_constant_string(buffer, "\0");
    int dirty = buffer->memory != old_ptr;
    if(dirty)
    {
        // NOTE: the memory has grown, fixup the pointers
        for(int i = 0; i < array->count; ++i)
        {
            array->elem[i] = buffer->memory + (array->elem[i] - old_ptr);
        }

    }
    reserve_string_array(array, 1);
    array->elem[array->count++] = buffer->memory + old_size;
}

static GrowableBuffer
escape_string(char *string)
{
    GrowableBuffer result = allocate_growable_buffer();
    for(char *c = string; *c; ++c)
    {
        switch(*c)
        {
            // NOTE: json escape sequences are from https://www.json.org/json-en.html
            case '\"': { write_constant_string(&result, "\\\""); } break;
            case '\\': { write_constant_string(&result, "\\\\"); } break;
            case '/': { write_constant_string(&result, "\\/"); } break;
            case '\b': { write_constant_string(&result, "\\b"); } break;
            case '\f': { write_constant_string(&result, "\\f"); } break;
            case '\n': { write_constant_string(&result, "\\n"); } break;
            case '\r': { write_constant_string(&result, "\\r"); } break;
            case '\t': { write_constant_string(&result, "\\t"); } break;
            default: { write_growable_buffer(&result, c, 1); } break;
        }
    }
    return result;
}

static int
find_index_case_insensitive(StringArray *array, char *keyword)
{
    int index = -1;
    for(int i = 0; i < array->count; ++i)
    {
        if(compare_case_insensitive(array->elem[i], keyword))
        {
            index = i;
            break;
        }
    }
    return index;
}

static StringArray
read_string_array_file(char *path)
{
    StringArray result = allocate_string_array();
    GrowableBuffer content = read_entire_file(path);
    for(size_t i = 0; i < content.used; ++i)
    {
        char *c = content.memory + i;
        if(*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n')
            *c = 0;
    }

    char prev_char = 0;
    for(size_t i = 0; i < content.used; ++i)
    {
        char *c = content.memory + i;
        if(!prev_char && *c)
            append_string_array(&result, c);

        prev_char = *c;
    }
    free_growable_buffer(&content);
    return result;
}

static int
find_label(Sheet *sheet, char *label)
{
    int result = -1;
    for(int i = 0; i < sheet->width; ++i)
    {
        if(compare_string(sheet->labels[i], label))
        {
            result = i;
            break;
        }
    }
    return result;
}

static char *
get_value(Sheet *sheet, int x, int y)
{
    assert(x < sheet->width && y < sheet->height);
    char *result = 0;
    if(x < sheet->width && y < sheet->height)
    {
        result = sheet->values[y * sheet->width + x];
    }
    return result;
}

static void
free_sheet(Sheet *sheet)
{
    free_memory(sheet->labels);
    free_memory(sheet->values);
    free_growable_buffer(&sheet->content);
    clear_memory(sheet, sizeof(*sheet));
}

static Sheet
allocate_sheet(int width, int height)
{
    Sheet result = {0};
    result.width = width;
    result.height = height;
    result.labels = (char **)allocate_memory(width * sizeof(*result.labels));
    result.values = (char **)allocate_memory(width * height * sizeof(*result.values));
    clear_memory(result.labels, width * sizeof(*result.labels));
    clear_memory(result.values, width * height * sizeof(*result.values));
    result.content = allocate_growable_buffer();
    return result;
}
