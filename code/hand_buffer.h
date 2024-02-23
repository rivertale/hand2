typedef struct GrowableBuffer
{
    size_t used;
    size_t max;
    char *memory;
} GrowableBuffer;

typedef struct StringArray
{
    int padding;
    int count;
    char **elem;
    GrowableBuffer buffer;
} StringArray;

typedef struct Sheet
{
    int width;
    int height;
    char **keys;
    char **values;
    GrowableBuffer content;
} Sheet;
