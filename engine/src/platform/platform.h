#pragma once

#include "defines.h"
#include "core/log.h"

bool platform_init(void* state, const char* app_name, i32 width, i32 height, i32 x, i32 y);
void platform_shutdown();

bool platform_handle_messages();

KENZINE_API void* platform_alloc(u64 size, bool aligned);
KENZINE_API void  platform_free(void* block, bool aligned);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, i32 value, u64 size);

void platform_console_write(const char* message, LogLevel level);
void platform_console_write_error(const char* message, LogLevel level);

f64  platform_get_absolute_time(void);
void platform_sleep(u64 ms);
u64 platform_get_state_size(void);