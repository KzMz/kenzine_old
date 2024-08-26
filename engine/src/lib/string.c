#include "lib/string.h"
#include "core/memory.h"
#include "lib/containers/dyn_array.h"

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
    memory_copy(copy, str, length);
    copy[length] = 0;
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

u32 string_split(const char* str, char delimiter, char*** str_darray, bool trim_entries, bool include_empty)
{
    if (str == NULL || str_darray == NULL)
    {
        return 0;
    }

    char* result = NULL;
    u32 trimmed_length = 0;
    u32 current_length = 0;
    u32 count = 0;
    u32 length = string_length(str);
    char buffer[16384];
    
    for (u32 i = 0; i < length; ++i)
    {
        char c = str[i];
        if (c == delimiter)
        {
            buffer[current_length] = 0;
            result = buffer;
            trimmed_length = current_length;
            
            if (trim_entries && current_length > 0)
            {
                result = string_trim(result);
                trimmed_length = string_length(result);
            }

            if (trimmed_length > 0 || include_empty)
            {
                char* entry = memory_alloc(sizeof(char) * (trimmed_length + 1), MEMORY_TAG_STRING);
                if (trimmed_length == 0)
                {
                    entry[0] = 0;
                }
                else 
                {
                    string_copy_n(entry, result, trimmed_length);
                    entry[trimmed_length] = 0;
                }
                char** a = *str_darray;
                dynarray_push(a, entry);
                *str_darray = a;
                count++;
            }

            memory_zero(buffer, 16384 * sizeof(char));
            current_length = 0;
            continue;
        }

        buffer[current_length++] = c;
    }

    result = buffer;
    trimmed_length = current_length;

    if (trim_entries && current_length > 0)
    {
        result = string_trim(result);
        trimmed_length = string_length(result);
    }
    if (trimmed_length > 0 || include_empty)
    {
        char* entry = memory_alloc(sizeof(char) * (trimmed_length + 1), MEMORY_TAG_STRING);
        if (trimmed_length == 0)
        {
            entry[0] = 0;
        }
        else 
        {
            string_copy_n(entry, result, trimmed_length);
            entry[trimmed_length] = 0;
        }
        char** a = *str_darray;
        dynarray_push(a, entry);
        *str_darray = a;
        count++;
    }

    return count;
}

void string_free_split(char** str_darray)
{
    if (str_darray == NULL)
    {
        return;
    }

    u32 count = dynarray_length(str_darray);
    for (u32 i = 0; i < count; ++i)
    {
        u32 len = string_length(str_darray[i]);
        memory_free(str_darray[i], (len + 1) * sizeof(char), MEMORY_TAG_STRING);
    }

    dynarray_destroy(str_darray);
}