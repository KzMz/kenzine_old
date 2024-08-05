#pragma once

#include "defines.h"

typedef struct Clock
{
    f64 start_time;
    f64 elapsed_time;
} Clock;

void clock_start(Clock* clock);
void clock_update(Clock* clock);
void clock_stop(Clock* clock);