#pragma once

#include <core/log.h>
#include <lib/math/math.h>

#define expect_eq(expected, actual)                                                                         \
    if (expected != actual)                                                                                 \
    {                                                                                                       \
        log_error("--> Expected %d, got %d. File: %s:%d", expected, actual, __FILE__, __LINE__);            \
        return false;                                                                                       \
    }

#define expect_not_eq(expected, actual)                                                                                  \
    if (expected == actual)                                                                                              \
    {                                                                                                                    \
        log_error("--> Expected %d != %d, but are equal. File: %s:%d", expected, actual, __FILE__, __LINE__);            \
        return false;                                                                                                    \
    }

#define expect_eq_f(expected, actual)                                                                                     \
    if (!math_abs(expected, actual) > 0.001f)                                                                             \
    {                                                                                                                     \
        log_error("--> Expected %f, got %f. File: %s:%d", expected, actual, __FILE__, __LINE__);                          \
        return false;                                                                                                     \
    }

#define expect_true(condition)                                                                                             \
    if (!condition)                                                                                                        \
    {                                                                                                                      \
        log_error("--> Expected true, got false. File: %s:%d", __FILE__, __LINE__);                                        \
        return false;                                                                                                      \
    }

#define expect_false(condition)                                                                                            \
    if (condition)                                                                                                         \
    {                                                                                                                      \
        log_error("--> Expected false, got true. File: %s:%d", __FILE__, __LINE__);                                        \
        return false;                                                                                                      \
    }