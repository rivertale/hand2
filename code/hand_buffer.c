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
    GrowableBuffer buffer = allocate_growable_buffer();
    FILE *file = fopen(path, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        size_t file_size = (size_t)ftell(file);
        fseek(file, 0, SEEK_SET);
        reserve_growable_buffer(&buffer, file_size);
        fread(buffer.memory + buffer.used, file_size, 1, file);
        buffer.used += file_size;
        fclose(file);
    }
    null_terminate(&buffer);
    return buffer;
}

static void
free_string_array(StringArray *array)
{
    free_growable_buffer(&array->buffer);
    free_memory(array->elem);
    clear_memory(array, sizeof(*array));
}

static StringArray
split_null_terminated_strings(GrowableBuffer *buffer)
{
    StringArray result = {0};
    char prev_char;

    prev_char = 0;
    for(size_t i = 0; i < buffer->used; ++i)
    {
        if(!prev_char) { ++result.count; }
        prev_char = buffer->memory[i];
    }

    result.buffer = *buffer;
    result.elem = (char **)allocate_memory(result.count * sizeof(char *));
    int index = 0;
    prev_char = 0;
    for(size_t i = 0; i < buffer->used; ++i)
    {
        char *c = buffer->memory + i;
        if(!prev_char) { result.elem[index++] = c; }
        prev_char = *c;
    }
    return result;
}

static StringArray
split_by_whitespace(GrowableBuffer *buffer)
{
    StringArray result = {0};
    char prev_char;

    prev_char = 0;
    for(size_t i = 0; i < buffer->used; ++i)
    {
        char *c = buffer->memory + i;
        if(*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n') { *c = 0; }

        if(!prev_char && *c) { ++result.count; }
        prev_char = *c;
    }

    result.buffer = *buffer;
    result.elem = (char **)allocate_memory(result.count * sizeof(char *));
    int index = 0;
    prev_char = 0;
    for(size_t i = 0; i < buffer->used; ++i)
    {
        char *c = buffer->memory + i;
        if(!prev_char && *c) { result.elem[index++] = c; }
        prev_char = *c;
    }
    return result;
}

static StringArray
escape_string_array(StringArray *array)
{
    GrowableBuffer buffer = allocate_growable_buffer();
    for(int i = 0; i < array->count; ++i)
    {
        for(char *c = array->elem[i]; *c; ++c)
        {
            switch(*c)
            {
                // NOTE: json escape sequences are from https://www.json.org/json-en.html
                case '\"': { write_constant_string(&buffer, "\\\""); } break;
                case '\\': { write_constant_string(&buffer, "\\\\"); } break;
                case '/': { write_constant_string(&buffer, "\\/"); } break;
                case '\b': { write_constant_string(&buffer, "\\b"); } break;
                case '\f': { write_constant_string(&buffer, "\\f"); } break;
                case '\n': { write_constant_string(&buffer, "\\n"); } break;
                case '\r': { write_constant_string(&buffer, "\\r"); } break;
                case '\t': { write_constant_string(&buffer, "\\t"); } break;
                default: { write_growable_buffer(&buffer, &c, 1); } break;
            }
        }
        write_constant_string(&buffer, "\0");
    }
    StringArray result = split_null_terminated_strings(&buffer);
    assert(result.count == array->count);
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

static GrowableBuffer
escape_growable_buffer(GrowableBuffer *buffer)
{
	GrowableBuffer result = allocate_growable_buffer();
	for(size_t i = 0; i < buffer->used; ++i)
	{
		char c = buffer->memory[i];
		switch(c)
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
			default: { write_growable_buffer(&result, &c, 1); } break;
		}
	}
	return result;
}

static StringArray
read_string_array_file(char *path)
{
    GrowableBuffer buffer = allocate_growable_buffer();

    FILE *file_handle = fopen(path, "rb");
    if(file_handle)
    {
        fseek(file_handle, 0, SEEK_END);
        size_t file_size = ftell(file_handle);
        fseek(file_handle, 0, SEEK_SET);

        reserve_growable_buffer(&buffer, file_size);
        if(fread(buffer.memory + buffer.used, file_size, 1, file_handle))
        {
            buffer.used += file_size;
        }
        fclose(file_handle);
    }
    else
    {
        write_log("file '%s' not exist", path);
    }
    null_terminate(&buffer);
    return split_by_whitespace(&buffer);
}

static int
find_key_index(Sheet *sheet, char *key)
{
    int result = -1;
    for(int i = 0; i < sheet->width; ++i)
    {
        if(compare_string(sheet->keys[i], key))
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
    free_memory(sheet->keys);
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
    result.keys = (char **)allocate_memory(width * sizeof(*result.keys));
    result.values = (char **)allocate_memory(width * height * sizeof(*result.values));
    clear_memory(result.keys, width * sizeof(*result.keys));
    clear_memory(result.values, width * height * sizeof(*result.values));
    result.content = allocate_growable_buffer();
    return result;
}