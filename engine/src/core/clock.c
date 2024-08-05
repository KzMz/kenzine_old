#include "clock.h"
#include "platform/platform.h"

void clock_start(Clock* clock)
{
    clock->start_time = platform_get_absolute_time();
    clock->elapsed_time = 0.0;
}

void clock_update(Clock* clock)
{
    if (clock->start_time == 0.0)
    {
        return;
    }

    f64 current_time = platform_get_absolute_time();
    clock->elapsed_time = current_time - clock->start_time;
}

void clock_stop(Clock* clock)
{
    clock->start_time = 0.0;
    clock->elapsed_time = 0.0;
}