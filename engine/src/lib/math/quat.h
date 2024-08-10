#pragma once

#include "math.h"
#include "mat4.h"
#include "vec3.h"

KENZINE_INLINE Quat quat_identity()
{
    return (Quat) {0, 0, 0, 1.0f};
}

KENZINE_INLINE f32 quat_normal(Quat q)
{
    return math_sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

KENZINE_INLINE Quat quat_normalized(Quat q)
{
    f32 n = quat_normal(q);
    return (Quat) {q.x / n, q.y / n, q.z / n, q.w / n};
}

KENZINE_INLINE Quat quat_conjugate(Quat q)
{
    return (Quat) {-q.x, -q.y, -q.z, q.w};
}

KENZINE_INLINE Quat quat_inverse(Quat q)
{
    return quat_normalized(quat_conjugate(q));
}

KENZINE_INLINE Quat quat_mul(Quat q0, Quat q1)
{
    Quat result = {0};

    result.x = q0.x * q1.w + 
               q0.y * q1.z - 
               q0.z * q1.y + 
               q0.w * q1.x;
    result.y = -q0.x * q1.z + 
                q0.y * q1.w + 
                q0.z * q1.x + 
                q0.w * q1.y;
    result.z = q0.x * q1.y -
               q0.y * q1.x +
               q0.z * q1.w +
               q0.w * q1.z;
    result.w = -q0.x * q1.x -
                q0.y * q1.y -
                q0.z * q1.z +
                q0.w * q1.w;

    return result;
}

KENZINE_INLINE f32 quat_dot(Quat q0, Quat q1)
{
    return q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
}

KENZINE_INLINE Mat4 quat_to_mat4(Quat q)
{
    Mat4 result = mat4_identity();
    Quat n = quat_normalized(q);

    result.elements[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    result.elements[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    result.elements[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

    result.elements[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    result.elements[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    result.elements[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

    result.elements[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
    result.elements[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
    result.elements[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return result;
}

KENZINE_INLINE Mat4 quat_to_rot_mat4(Quat q, Vec3 center)
{
    Mat4 result = {0};

    f32* m = result.elements;
    m[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    m[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
    m[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
    m[3] = center.x - center.x * m[0] - center.y * m[1] - center.z * m[2];

    m[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
    m[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    m[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
    m[7] = center.y - center.x * m[4] - center.y * m[5] - center.z * m[6];

    m[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
    m[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
    m[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    m[11] = center.z - center.x * m[8] - center.y * m[9] - center.z * m[10];

    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    return result;
}

KENZINE_INLINE Quat quat_from_axis_angle(Vec3 axis, f32 angle, bool normalize)
{
    f32 half_angle = angle * 0.5f;
    f32 s = math_sin(half_angle);
    f32 c = math_cos(half_angle);

    Quat result = {axis.x * s, axis.y * s, axis.z * s, c};
    return normalize ? quat_normalized(result) : result;
}

KENZINE_INLINE Quat quat_slerp(Quat q0, Quat q1, f32 percentage)
{
    Quat result = {0};
    Quat v0 = quat_normalized(q0);
    Quat v1 = quat_normalized(q1);

    f32 dot = quat_dot(v0, v1);
    if (dot < 0.0f)
    {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD)
    {
        result.x = v0.x + percentage * (v1.x - v0.x);
        result.y = v0.y + percentage * (v1.y - v0.y);
        result.z = v0.z + percentage * (v1.z - v0.z);
        result.w = v0.w + percentage * (v1.w - v0.w);
        return quat_normalized(result);
    }

    f32 theta_0 = math_acos(dot);
    f32 theta = theta_0 * percentage;
    f32 sin_theta = math_sin(theta);
    f32 sin_theta_0 = math_sin(theta_0);

    f32 s0 = math_cos(theta) - dot * sin_theta / sin_theta_0;
    f32 s1 = sin_theta / sin_theta_0;

    result.x = (s0 * v0.x) + (s1 * v1.x);
    result.y = (s0 * v0.y) + (s1 * v1.y);
    result.z = (s0 * v0.z) + (s1 * v1.z);
    result.w = (s0 * v0.w) + (s1 * v1.w);
    return result;
}