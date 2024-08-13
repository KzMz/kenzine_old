#include "log.h"
#include "asserts.h"
#include "platform/platform.h"
#include "core/memory.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

typedef struct LoggerState
{
    bool initialized;
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

bool log_init(void* state) 
{
    logger_state = state;
    logger_state->initialized = true;
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
    mem_zero(out_buffer);

    __builtin_va_list args;
    va_start(args, message);
    vsnprintf(out_buffer, LOG_BUFFER_SIZE, message, args);
    va_end(args);

    char out_buffer_final[LOG_BUFFER_SIZE];
    mem_zero(out_buffer_final);

    sprintf(out_buffer_final, "%s%s\n", level_strings_prefix[level], out_buffer);

    if (is_error)
    {
        platform_console_write_error(out_buffer_final, level);
    }
    else
    {
        platform_console_write(out_buffer_final, level);
    }
}