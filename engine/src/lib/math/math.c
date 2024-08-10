#include "math.h"
#include "platform/platform.h"

#include <math.h>
#include <stdlib.h>

static bool rand_seeded = false;

f32 math_sin(f32 x)
{
    return sinf(x);
}

f32 math_cos(f32 x)
{
    return cosf(x);
}

f32 math_tan(f32 x)
{
    return tanf(x);
}

f32 math_acos(f32 x)
{
    return acosf(x);
}

f32 math_sqrt(f32 x)
{
    return sqrtf(x);
}

f32 math_abs(f32 x)
{
    return fabsf(x);
}

i32 math_irandom()
{
    if (!rand_seeded)
    {
        srand((u32) platform_get_absolute_time());
        rand_seeded = true;
    }
    return rand();
}

i32 math_irandom_range(i32 min, i32 max)
{
    i32 rand = math_irandom();
    return min + (rand % (max - min + 1));
}

f32 math_frandom()
{
    return (f32) math_irandom() / (f32) RAND_MAX;
}

f32 math_frandom_range(f32 min, f32 max)
{
    f32 rand = math_frandom();
    return min + rand * (max - min);
}