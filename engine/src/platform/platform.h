#pragma once

#include "defines.h"
#include "core/log.h"

struct FileHandle;

typedef struct PlatformHIDDevice
{
    void* device_handle;
    void* output_handle;
    char name[1024];
    char product[128];
    u32 product_id;
    char manufacturer[128];
    u32 vendor_id;
    char serial_number[128];
} PlatformHIDDevice;

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

void platform_sleep(u64 ms);
f64 platform_get_absolute_time(void);
u64 platform_get_state_size(void);

bool platform_register_hid_device(void);
void platform_create_hid_device(void* handle, PlatformHIDDevice* out_device);
void platform_destroy_hid_device(PlatformHIDDevice* device);