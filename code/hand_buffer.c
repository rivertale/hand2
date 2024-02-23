#define write_constant_string(buffer, array) write_growable_buffer(buffer, array, sizeof(array) - 1)

static GrowableBuffer
allocate_growable_buffer(void)
{
    size_t initial_size = 4096;
    GrowableBuffer result = {0};
    result.max = initial_size;
    result.memory = allocate_memory(initial_size);
    return result;
}

static void
free_growable_buffer(GrowableBuffer *buffer)
{
    if(buffer->memory)
    {
        free(buffer->memory);
    }
    clear_memory(buffer, sizeof(*buffer));
}

static void
reserve_growable_buffer(GrowableBuffer *buffer, size_t size)
{
    if(buffer->used + size > buffer->max)
    {
        size_t allocated_size = next_pow_of_two(buffer->used + size);
        char *memory = allocate_memory(allocated_size);

        copy_memory(memory, buffer->memory, buffer->used);
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
}

static void
null_terminate(GrowableBuffer *buffer)
{
    if(buffer->used > 0 && buffer->memory[buffer->used - 1] != '\0')
    {
        char null_terminator = '\0';
        write_growable_buffer(buffer, &null_terminator, sizeof(null_terminator));
    }
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
    null_terminate(buffer);

    for(size_t i = 0; i < buffer->used; ++i)
    {
        if(buffer->memory[i] == '\0') ++result.count;
    }

    result.buffer = *buffer;
    result.elem = (char **)allocate_memory(result.count * sizeof(char *));
    char prev_char = 0;
    int index = 0;
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
    null_terminate(buffer);

    char prev_char = 0;
    for(size_t i = 0; i < buffer->used; ++i)
    {
        char *c = buffer->memory + i;
        if(*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n') { *c = 0; }

        if(!prev_char && *c) { ++result.count; }
        prev_char = *c;
    }

    result.buffer = *buffer;
    result.elem = (char **)allocate_memory(result.count * sizeof(char *));
    prev_char = 0;
    int index = 0;
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
                case '\"': { write_growable_buffer(&buffer, "\\\"", 2); } break;
                case '\\': { write_growable_buffer(&buffer, "\\", 2); } break;
                case '/': { write_growable_buffer(&buffer, "\\/", 2); } break;
                case '\b': { write_growable_buffer(&buffer, "\\b", 2); } break;
                case '\f': { write_growable_buffer(&buffer, "\\f", 2); } break;
                case '\n': { write_growable_buffer(&buffer, "\\n", 2); } break;
                case '\r': { write_growable_buffer(&buffer, "\\r", 2); } break;
                case '\t': { write_growable_buffer(&buffer, "\\t", 2); } break;
                default: { write_growable_buffer(&buffer, &c, 1); } break;
            }
        }
        write_constant_string(&buffer, "\0");
    }
    StringArray result = split_null_terminated_strings(&buffer);
    assert(result.count == array->count);
    return result;
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
			case '\"': { write_growable_buffer(&result, "\\\"", 2); } break;
			case '\\': { write_growable_buffer(&result, "\\", 2); } break;
			case '/': { write_growable_buffer(&result, "\\/", 2); } break;
			case '\b': { write_growable_buffer(&result, "\\b", 2); } break;
			case '\f': { write_growable_buffer(&result, "\\f", 2); } break;
			case '\n': { write_growable_buffer(&result, "\\n", 2); } break;
			case '\r': { write_growable_buffer(&result, "\\r", 2); } break;
			case '\t': { write_growable_buffer(&result, "\\t", 2); } break;
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