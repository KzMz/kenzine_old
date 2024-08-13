#pragma once

#include "defines.h"

#define LOG_WARNING_ENABLED true
#define LOG_INFO_ENABLED true

#if KZRELEASE == false
    #define LOG_DEBUG_ENABLED true
    #define LOG_TRACE_ENABLED true
#else
    #define LOG_DEBUG_ENABLED false
    #define LOG_TRACE_ENABLED false
#endif

#define LOG_BUFFER_SIZE 32 * 1024

#define LOG_ERROR_COLOR "[0;31m"
#define LOG_WARNING_COLOR "[0;33m"

typedef enum LogLevel
{
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
} LogLevel;

u64 log_state_size();
bool log_init(void* state);
void log_shutdown(void);

KENZINE_API void log_message(LogLevel level, const char* message, ...);

#define log_fatal(message, ...) log_message(LOG_LEVEL_FATAL, message, ##__VA_ARGS__)
#define log_error(message, ...) log_message(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)

#if LOG_WARNING_ENABLED == true
    #define log_warning(message, ...) log_message(LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#else
    #define log_warning(message, ...)
#endif

#if LOG_INFO_ENABLED == true
    #define log_info(message, ...) log_message(LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#else
    #define log_info(message, ...)
#endif

#if LOG_DEBUG_ENABLED == true
    #define log_debug(message, ...) log_message(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#else
    #define log_debug(message, ...)
#endif

#if LOG_TRACE_ENABLED == true
    #define log_trace(message, ...) log_message(LOG_LEVEL_TRACE, message, ##__VA_ARGS__)
#else
    #define log_trace(message, ...)
#endif