#pragma once

#include "defines.h"

#define MAX_STRING_BUFFER_SIZE 32000

KENZINE_API u64 string_length(const char* str);
KENZINE_API char* string_clone(const char* str);
KENZINE_API bool string_equals(const char* str1, const char* str2);
KENZINE_API bool string_equals_nocase(const char* str1, const char* str2);
KENZINE_API i32 string_format(char* dest, const char* format, ...);
KENZINE_API i32 string_format_v(char* dest, const char* format, void* args);