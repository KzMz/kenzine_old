#include "log.h"
#include "asserts.h"
#include "platform/platform.h"
#include "platform/filesystem.h"
#include "core/memory.h"
#include "lib/string.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_FILE_NAME "console.log"

typedef struct LoggerState
{
    FileHandle log_file;
} LoggerState;

static LoggerState* logger_state = NULL;

void kz_assert_failure(const char* expression, const char* message, const char* file, i32 line)
{
    log_fatal("Assertion failed: %s\nMessage: %s\nFile: %s\nLine: %d", expression, message, file, line);
}

u64 log_state_size()
{
    return sizeof(LoggerState);
}

void append_to_log_file(const char* message)
{
    if (logger_state == NULL)
    {
        return;
    }

    u64 length = string_length(message);
    u64 written = 0;
    if (!file_write(&logger_state->log_file, length, message, &written))
    {
        platform_console_write_error("Failed to write to log file", LOG_LEVEL_ERROR);
    }
}

bool log_init(void* state) 
{
    logger_state = state;

    if (!file_open(LOG_FILE_NAME, FILE_MODE_WRITE, false, &logger_state->log_file))
    {
        platform_console_write_error("Failed to open log file", LOG_LEVEL_ERROR);
        return false;
    }

    return true;
}

void log_shutdown(void)
{
    logger_state = NULL;
}

KENZINE_API void log_message(LogLevel level, const char* message, ...)
{
    const char* level_strings_prefix[] = 
    {
        "[FATAL]:\t",
        "[ERROR]:\t",
        "[WARNING]:\t",
        "[INFO]:\t\t",
        "[DEBUG]:\t",
        "[TRACE]:\t"
    };

    const bool is_error = level == LOG_LEVEL_FATAL || level == LOG_LEVEL_ERROR;

    char out_buffer[LOG_BUFFER_SIZE];
    memory_zero(out_buffer, LOG_BUFFER_SIZE);

    __builtin_va_list args;
    va_start(args, message);
    string_format_v(out_buffer, message, args);
    //vsnprintf(out_buffer, LOG_BUFFER_SIZE, message, args);
    va_end(args);

    //char out_buffer_final[LOG_BUFFER_SIZE];
    //mem_zero(out_buffer_final);

    //sprintf(out_buffer_final, "%s%s\n", level_strings_prefix[level], out_buffer);
    string_format(out_buffer, "%s%s\n", level_strings_prefix[level], out_buffer);

    if (is_error)
    {
        platform_console_write_error(out_buffer, level);
    }
    else
    {
        platform_console_write(out_buffer, level);
    }

    append_to_log_file(out_buffer);
}