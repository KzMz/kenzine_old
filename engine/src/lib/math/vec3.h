#pragma once 

#include "math.h"

KENZINE_INLINE Vec3 vec3_create(f32 x, f32 y, f32 z)
{
    Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

KENZINE_INLINE Vec3 vec3_from_vec4(Vec4 v)
{
    return (Vec3) {v.x, v.y, v.z};
}

KENZINE_INLINE Vec4 vec3_to_vec4(Vec3 v, f32 w)
{
    return (Vec4) {v.x, v.y, v.z, w};
}

KENZINE_INLINE Vec3 vec3_zero()
{
    return (Vec3) {0.0f, 0.0f, 0.0f};
}

KENZINE_INLINE Vec3 vec3_one()
{
    return (Vec3) {1.0f, 1.0f, 1.0f};
}

KENZINE_INLINE Vec3 vec3_up()
{
    return (Vec3) {0.0f, 1.0f, 0.0f};
}

KENZINE_INLINE Vec3 vec3_down()
{
    return (Vec3) {0.0f, -1.0f, 0.0f};
}

KENZINE_INLINE Vec3 vec3_left()
{
    return (Vec3) {-1.0f, 0.0f, 0.0f};
}

KENZINE_INLINE Vec3 vec3_right()
{
    return (Vec3) {1.0f, 0.0f, 0.0f};
}

KENZINE_INLINE Vec3 vec3_forward()
{
    return (Vec3) {0.0f, 0.0f, -1.0f};
}

KENZINE_INLINE Vec3 vec3_back()
{
    return (Vec3) {0.0f, 0.0f, 1.0f};
}

KENZINE_INLINE Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return (Vec3) 
    {
        a.x + b.x, 
        a.y + b.y, 
        a.z + b.z
    };
}

KENZINE_INLINE Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    return (Vec3) 
    {
        a.x - b.x, 
        a.y - b.y, 
        a.z - b.z
    };
}

KENZINE_INLINE Vec3 vec3_mul(Vec3 a, Vec3 b)
{
    return (Vec3) 
    {
        a.x * b.x, 
        a.y * b.y, 
        a.z * b.z
    };
}

KENZINE_INLINE Vec3 vec3_mul_scalar(Vec3 v, f32 s)
{
    return (Vec3) 
    {
        v.x * s, 
        v.y * s, 
        v.z * s
    };
}

KENZINE_INLINE Vec3 vec3_div(Vec3 a, Vec3 b)
{
    return (Vec3) 
    {
        a.x / b.x, 
        a.y / b.y, 
        a.z / b.z
    };
}

KENZINE_INLINE f32 vec3_length_squared(Vec3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

KENZINE_INLINE f32 vec3_length(Vec3 v)
{
    return math_sqrt(vec3_length_squared(v));
}

KENZINE_INLINE Vec3 vec3_normalize(Vec3* v)
{
    f32 length = vec3_length(*v);
    v->x /= length;
    v->y /= length;
    v->z /= length;
    return *v;
}

KENZINE_INLINE Vec3 vec3_normalized(Vec3 v)
{
    vec3_normalize(&v);
    return v;
}

KENZINE_INLINE f32 vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

KENZINE_INLINE Vec3 vec3_cross(Vec3 a, Vec3 b)
{
    return (Vec3) 
    {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

KENZINE_INLINE bool vec3_equals(Vec3 a, Vec3 b, f32 tolerance)
{
    if (math_abs(a.x - b.x) > tolerance)
    {
        return false;
    }

    if (math_abs(a.y - b.y) > tolerance)
    {
        return false;
    }

    if (math_abs(a.z - b.z) > tolerance)
    {
        return false;
    }

    return true;
}

KENZINE_INLINE f32 vec3_distance(Vec3 a, Vec3 b)
{
    Vec3 delta = vec3_sub(a, b);
    return vec3_length(delta);
}

KENZINE_INLINE f32 vec3_distance_squared(Vec3 a, Vec3 b)
{
    Vec3 delta = vec3_sub(a, b);
    return vec3_length_squared(delta);
}