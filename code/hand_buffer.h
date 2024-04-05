typedef struct GrowableBuffer
{
    size_t used;
    size_t max;
    char *memory;
} GrowableBuffer;

typedef struct StringArray
{
    int count;
    int max;
    char **elem;
    GrowableBuffer buffer;
} StringArray;

typedef struct Sheet
{
    int width;
    int height;
    char **labels;
    char **values;
    GrowableBuffer content;
} Sheet;
