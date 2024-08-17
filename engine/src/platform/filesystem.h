#pragma once

#include "defines.h"

typedef struct FileHandle
{
    void* handle;
    bool valid;
} FileHandle;

typedef enum FileMode
{
    FILE_MODE_READ = 0x1,
    FILE_MODE_WRITE = 0x2,
} FileMode;

KENZINE_API bool file_exists(const char* path);
KENZINE_API bool file_open(const char* path, FileMode mode, bool binary, FileHandle* out_handle);
KENZINE_API void file_close(FileHandle* handle);

KENZINE_API bool file_read_line(FileHandle* handle, u64 max_length, char** line_buf, u64* out_length);
KENZINE_API bool file_get_contents(FileHandle* handle, char* out_contents, u64* out_size);

KENZINE_API bool file_write_line(FileHandle* handle, const char* line);

KENZINE_API bool file_read(FileHandle* handle, u64 size, void* out_data, u64* out_actual_size);
KENZINE_API bool file_read_all_bytes(FileHandle* handle, u8** out_bytes, u64* out_actual_size);

KENZINE_API bool file_write(FileHandle* handle, u64 size, const void* data, u64* out_actual_size);