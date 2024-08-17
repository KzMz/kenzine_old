#include "lib/string.h"
#include "core/memory.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#ifndef _MSC_VER
#include <strings.h>
#endif

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

bool string_equals_nocase(const char* str1, const char* str2)
{
#if defined(__GNUC__)
    return strcasecmp(str1, str2) == 0;
#elif defined(_MSC_VER)
    return _strcmpi(str1, str2) == 0;
#endif
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

char* string_copy(char* dest, const char* src)
{
    return strcpy(dest, src);
}

char* string_copy_n(char* dest, const char* src, u64 n)
{
    return strncpy(dest, src, n);
}

char* string_trim(char* str)
{
    while (isspace((unsigned char) *str))
    {
        str++;
    }

    if (*str == 0)
    {
        return str;
    }

    char* p = str;
    while (*p != 0)
    {
        p++;
    }

    while (isspace((unsigned char)*(--p)));

    p[1] = '\0';

    return str;
}

void string_mid(char* dest, const char* src, u64 start, u64 count)
{
    if (count == 0)
    {
        return;
    }

    u64 length = string_length(src);
    if (start >= length)
    {
        dest[0] = 0;
        return;
    }

    if (count > 0)
    {
        for (u64 i = start, j = 0; j < length && src[i] != 0; ++i, ++j)
        {
            dest[j] = src[i];
        }
        dest[start + count] = 0;
    }
    else 
    {
        u64 j = 0;
        for (u64 i = start; src[i] != 0; ++i, ++j)
        {
            dest[j] = src[i];
        }
        dest[start + j] = 0;
    }
}

char* string_empty(char* str)
{
    if (str == NULL)
    {
        return NULL;
    }

    str[0] = 0;
    return str;
}