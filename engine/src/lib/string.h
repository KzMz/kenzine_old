#pragma once

#include "defines.h"

#define MAX_STRING_BUFFER_SIZE 32000

KENZINE_API u64 string_length(const char* str);
KENZINE_API char* string_clone(const char* str);
KENZINE_API bool string_equals(const char* str1, const char* str2);
KENZINE_API bool string_equals_nocase(const char* str1, const char* str2);
KENZINE_API i32 string_format(char* dest, const char* format, ...);
KENZINE_API i32 string_format_v(char* dest, const char* format, void* args);
KENZINE_API char* string_copy(char* dest, const char* src);
KENZINE_API char* string_copy_n(char* dest, const char* src, u64 n);
KENZINE_API char* string_trim(char* str);
KENZINE_API void string_mid(char* dest, const char* src, u64 start, u64 count);