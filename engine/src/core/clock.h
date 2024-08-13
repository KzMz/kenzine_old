#pragma once

#include "defines.h"

typedef struct Clock
{
    f64 start_time;
    f64 elapsed_time;
} Clock;

KENZINE_API void clock_start(Clock* clock);
KENZINE_API void clock_update(Clock* clock);
KENZINE_API void clock_stop(Clock* clock);