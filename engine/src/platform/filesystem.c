#include "filesystem.h"
#include "core/log.h"
#include "core/memory.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool file_exists(const char* path)
{
#ifdef _MSC_VER
    struct _stat buffer;
    return _stat(path, &buffer);
#else
    struct stat buffer;
    return stat(path, &buffer) == 0;
#endif
}

bool file_open(const char* path, FileMode mode, bool binary, FileHandle* out_handle)
{
    out_handle->valid = false;
    out_handle->handle = NULL;

    const char* mode_str = NULL;
    if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0)
    {
        mode_str = binary ? "w+b" : "w+";
    }
    else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0)
    {
        mode_str = binary ? "rb" : "r";
    }
    else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0)
    {
        mode_str = binary ? "wb" : "w";
    }
    else
    {
        log_error("Invalid file mode");
        return false;
    }

    FILE* file = fopen(path, mode_str);
    if (file == NULL)
    {
        log_error("Failed to open file: %s", path);
        return false;
    }

    out_handle->valid = true;
    out_handle->handle = file;
    return true;
}

void file_close(FileHandle* handle)
{
    if (handle->valid)
    {
        fclose((FILE*) handle->handle);
        handle->valid = false;
        handle->handle = NULL;
    }
}

bool file_size(FileHandle* handle, u64* out_size)
{
    if (handle == NULL || !handle->valid)
    {
        return false;
    }

    FILE* file = (FILE*) handle->handle;
    fseek(file, 0, SEEK_END);
    *out_size = ftell(file);
    rewind(file);
    return true;
}

bool file_read_line(FileHandle* handle, u64 max_length, char** line_buf, u64* out_length)
{
    if (!handle->valid)
    {
        log_error("Invalid file handle");
        return false;
    }

    char* buf = *line_buf;
    if (fgets(buf, max_length, (FILE*) handle->handle) != 0)
    {
        *out_length = strlen(*line_buf);
        return true;
    }

    return false;
}

bool file_get_contents(FileHandle* handle, char* out_contents, u64* out_size)
{
    if (!handle->valid)
    {
        log_error("Invalid file handle");
        return false;
    }

    u64 size = 0;
    if (!file_size(handle, &size))
    {
        return false;
    }

    FILE* file = (FILE*) handle->handle;
    *out_size = fread(out_contents, 1, size, file);
    return *out_size == size;
}

bool file_write_line(FileHandle* handle, const char* line)
{
    if (!handle->valid)
    {
        log_error("Invalid file handle");
        return false;
    }

    FILE* file = (FILE*) handle->handle;
    i32 result = fputs(line, file);
    if (result != EOF)
    {
        result = fputc('\n', file);
    }

    fflush(file);
    return result != EOF;
}

bool file_read(FileHandle* handle, u64 size, void* out_data, u64* out_actual_size)
{
    if (!handle->valid)
    {
        log_error("Invalid file handle");
        return false;
    }

    FILE* file = (FILE*) handle->handle;
    *out_actual_size = fread(out_data, 1, size, file);
    return *out_actual_size == size;
}

bool file_read_all_bytes(FileHandle* handle, u8* out_bytes, u64* out_actual_size)
{
    if (!handle->valid)
    {
        log_error("Invalid file handle");
        return false;
    }

    u64 size = 0;
    if (!file_size(handle, &size))
    {
        return false;
    }

    FILE* file = (FILE*) handle->handle;
    *out_actual_size = fread(out_bytes, 1, size, file);
    return *out_actual_size == size;
}

bool file_write(FileHandle* handle, u64 size, const void* data, u64* out_actual_size)
{
    if (!handle->valid)
    {
        log_error("Invalid file handle");
        return false;
    }

    FILE* file = (FILE*) handle->handle;
    *out_actual_size = fwrite(data, 1, size, file);
    fflush(file);
    return *out_actual_size == size;
}