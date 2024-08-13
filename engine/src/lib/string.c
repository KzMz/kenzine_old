#include "lib/string.h"
#include "core/memory.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

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

i32 string_format(char* dest, const char* format, ...)
{
    if (dest == NULL || format == NULL)
    {
        return -1;
    }

    __builtin_va_list args;
    va_start(args, format);
    i32 result = string_format_v(dest, format, args);
    va_end(args);
    return result;
}

i32 string_format_v(char* dest, const char* format, void* args)
{
    if (dest == NULL || format == NULL)
    {
        return -1;
    }

    char buffer[MAX_STRING_BUFFER_SIZE];
    i32 result = vsnprintf(buffer, MAX_STRING_BUFFER_SIZE, format, args);
    buffer[result] = '\0';
    memory_copy(dest, buffer, result + 1);

    return result;
}