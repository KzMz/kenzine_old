#pragma once

#include "math.h"

KENZINE_INLINE Vec4 vec4_create(f32 x, f32 y, f32 z, f32 w)
{
    Vec4 result;
#if defined(KZ_USE_SIMD)
    result.data = _mm_setr_ps(w, z, y, x);
#else
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
#endif
    return result;
}

KENZINE_INLINE Vec4 vec4_from_vec3(Vec3 v, f32 w)
{
#if defined(KZ_USE_SIMD)
    Vec4 result;
    result.data = _mm_setr_ps(w, v.z, v.y, v.x);
    return result;
#else
    return (Vec4) {v.x, v.y, v.z, w};
#endif
}

KENZINE_INLINE Vec3 vec4_to_vec3(Vec4 v)
{
    return (Vec3) {v.x, v.y, v.z};
}

KENZINE_INLINE Vec4 vec4_zero()
{
    return (Vec4) {0.0f, 0.0f, 0.0f, 0.0f};
}

KENZINE_INLINE Vec4 vec4_one()
{
    return (Vec4) {1.0f, 1.0f, 1.0f, 1.0f};
}

KENZINE_INLINE Vec4 vec4_add(Vec4 a, Vec4 b)
{
    Vec4 result;
    for (u32 i = 0; i < 4; ++i)
    {
        result.elements[i] = a.elements[i] + b.elements[i];
    }
    return result;
}

KENZINE_INLINE Vec4 vec4_sub(Vec4 a, Vec4 b)
{
    Vec4 result;
    for (u32 i = 0; i < 4; ++i)
    {
        result.elements[i] = a.elements[i] - b.elements[i];
    }
    return result;
}

KENZINE_INLINE Vec4 vec4_mul(Vec4 a, Vec4 b)
{
    Vec4 result;
    for (u32 i = 0; i < 4; ++i)
    {
        result.elements[i] = a.elements[i] * b.elements[i];
    }
    return result;
}

KENZINE_INLINE Vec4 vec4_div(Vec4 a, Vec4 b)
{
    Vec4 result;
    for (u32 i = 0; i < 4; ++i)
    {
        result.elements[i] = a.elements[i] / b.elements[i];
    }
    return result;
}

KENZINE_INLINE f32 vec4_length_squared(Vec4 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

KENZINE_INLINE f32 vec4_length(Vec4 v)
{
    return math_sqrt(vec4_length_squared(v));
}

KENZINE_INLINE Vec4 vec4_normalize(Vec4* v)
{
    f32 length = vec4_length(*v);
    kz_assert(length > 0);

    v->x /= length;
    v->y /= length;
    v->z /= length;
    v->w /= length;
}

KENZINE_INLINE Vec4 vec4_normalized(Vec4 v)
{
    vec4_normalize(&v);
    return v;
}

KENZINE_INLINE f32 vec4_fdot(
    f32 x0, f32 y0, f32 z0, f32 w0,
    f32 x1, f32 y1, f32 z1, f32 w1)
{
    return x0 * x1 + y0 * y1 + z0 * z1 + w0 * w1;
}