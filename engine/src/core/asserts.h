#pragma once

#include "defines.h"

#define KZASSERTIONS_ENABLED true

#if KZASSERTIONS_ENABLED == true
    #if _MSC_VER
        #include <intrin.h>
        #define debug_break() __debugbreak()
    #else
        #define debug_break() __builtin_trap()
    #endif

    KENZINE_API void kz_assert_failure(const char* expression, const char* message, const char* file, i32 line); // log.c

    #define kz_assert(expression)                                               \
        do {                                                                    \
            if ((expression)) { }                                               \
            else {                                                              \
                kz_assert_failure(#expression, "", __FILE__, __LINE__);         \
                debug_break();                                                  \
            }                                                                   \
        } while (0)

    #define kz_assert_msg(expression, message)                                  \
        do {                                                                    \
            if ((expression)) { }                                               \
            else {                                                              \
                kz_assert_failure(#expression, message, __FILE__, __LINE__);    \
                debug_break();                                                  \
            }                                                                   \
        } while (0)

    #ifdef _DEBUG
        #define kz_assert_debug(expression)                                               \
            do {                                                                    \
                if ((expression)) { }                                               \
                else {                                                              \
                    kz_assert_failure(#expression, "", __FILE__, __LINE__);         \
                    debug_break();                                                  \
                }                                                                   \
            } while (0)

        #define kz_assert_debug_msg(expression, message)                                  \
            do {                                                                    \
                if ((expression)) { }                                               \
                else {                                                              \
                    kz_assert_failure(#expression, message, __FILE__, __LINE__);    \
                    debug_break();                                                  \
                }                                                                   \
            } while (0)
    #else 
        #define kz_assert_debug(expression)
        #define kz_assert_msg_debug(expression, message)
    #endif
#else
    #define kz_assert(expression)
    #define kz_assert_msg(expression, message)
    #define kz_assert_debug(expression)
    #define kz_assert_debug_msg(expression, message)
#endif