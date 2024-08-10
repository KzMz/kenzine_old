#pragma once

#include "math.h"

KENZINE_INLINE Vec2 vec2_create(f32 x, f32 y)
{
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

KENZINE_INLINE Vec2 vec2_zero()
{
    return (Vec2) {0.0f, 0.0f};
}

KENZINE_INLINE Vec2 vec2_one()
{
    return (Vec2) {1.0f, 1.0f};
}

KENZINE_INLINE Vec2 vec2_up()
{
    return (Vec2) {0.0f, 1.0f};
}

KENZINE_INLINE Vec2 vec2_down()
{
    return (Vec2) {0.0f, -1.0f};
}

KENZINE_INLINE Vec2 vec2_left()
{
    return (Vec2) {-1.0f, 0.0f};
}

KENZINE_INLINE Vec2 vec2_right()
{
    return (Vec2) {1.0f, 0.0f};
}

KENZINE_INLINE Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return (Vec2) 
    {
        a.x + b.x, 
        a.y + b.y
    };
}

KENZINE_INLINE Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return (Vec2) 
    {
        a.x - b.x, 
        a.y - b.y
    };
}

KENZINE_INLINE Vec2 vec2_mul(Vec2 a, Vec2 b)
{
    return (Vec2) 
    {
        a.x * b.x, 
        a.y * b.y
    };
}

KENZINE_INLINE Vec2 vec2_div(Vec2 a, Vec2 b)
{
    return (Vec2) 
    {
        a.x / b.x, 
        a.y / b.y
    };
}

KENZINE_INLINE f32 vec2_length_squared(Vec2 v)
{
    return v.x * v.x + v.y * v.y;
}

KENZINE_INLINE f32 vec2_length(Vec2 v)
{
    return math_sqrt(vec2_length_squared(v));
}

KENZINE_INLINE Vec2 vec2_normalize(Vec2* v)
{
    f32 length = vec2_length(*v);
    kz_assert(length > 0.0f);

    v->x /= length;
    v->y /= length;
}

KENZINE_INLINE Vec2 vec2_normalized(Vec2 v)
{
    vec2_normalize(&v);
    return v;
}

KENZINE_INLINE bool vec2_equals(Vec2 a, Vec2 b, f32 tolerance)
{
    if (math_abs(a.x - b.x) > tolerance)
    {
        return false;
    }

    if (math_abs(a.y - b.y) > tolerance)
    {
        return false;
    }

    return true;
}

KENZINE_INLINE f32 vec2_distance(Vec2 a, Vec2 b)
{
    Vec2 delta = vec2_sub(a, b);
    return vec2_length(delta);
}

KENZINE_INLINE f32 vec2_distance_squared(Vec2 a, Vec2 b)
{
    Vec2 delta = vec2_sub(a, b);
    return vec2_length_squared(delta);
}