#pragma once

#include "defines.h"
#include "math_defines.h"
#include "core/asserts.h"

#define KZ_PI 3.14159265358979323846f
#define KZ_PI_DOUBLE KI_PI * 2.0f
#define KZ_PI_HALF KZ_PI * 0.5f
#define KZ_PI_QUARTER KZ_PI * 0.25f
#define KZ_PI_ONE_OVER 1.0f / KZ_PI
#define KZ_PI_DOUBLE_ONE_OVER 1.0f / KZ_PI_DOUBLE

#define KZ_SQRT_TWO 1.41421356237309504880f
#define KZ_SQRT_THREE 1.73205080756887729352f
#define KZ_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define KZ_SQRT_ONE_OVER_THREE 0.57735026918962576450f

#define KZ_DEG2RAD KZ_PI / 180.0f
#define KZ_RAD2DEG 180.0f / KZ_PI

#define KZ_SEC2MS 1000.0f
#define KZ_MS2SEC 0.001f

#define KZ_INFINITY 1e30f
#define KZ_EPSILON 1.192092896e-07f

KENZINE_API f32 math_sin(f32 x);
KENZINE_API f32 math_cos(f32 x);
KENZINE_API f32 math_tan(f32 x);
KENZINE_API f32 math_acos(f32 x);
KENZINE_API f32 math_sqrt(f32 x);
KENZINE_API f32 math_abs(f32 x);

KENZINE_INLINE bool is_power_of_two(u64 x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

KENZINE_API i32 math_irandom();
KENZINE_API i32 math_irandom_range(i32 min, i32 max);

KENZINE_API f32 math_frandom();
KENZINE_API f32 math_frandom_range(f32 min, f32 max);

KENZINE_INLINE f32 deg_to_rad(f32 degrees)
{
    return degrees * KZ_DEG2RAD;
}

KENZINE_INLINE f32 rad_to_deg(f32 radians)
{
    return radians * KZ_RAD2DEG;
}