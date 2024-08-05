#include "lib/string.h"
#include "core/memory.h"

#include <string.h>

u64 string_length(const char* str)
{
    return strlen(str);
}

char* string_clone(const char* str)
{
    u64 length = string_length(str);
    char* copy = memory_alloc(length + 1, MEMORY_TAG_STRING);
    memory_copy(copy, str, length + 1);
    return copy;
}

bool string_equals(const char* str1, const char* str2)
{
    return strcmp(str1, str2) == 0;
}