#pragma once

#include "math.h"
#include "vec3.h"

KENZINE_INLINE Mat4 mat4_identity()
{
    Mat4 result = {0};
    result.elements[0] = 1.0f;
    result.elements[5] = 1.0f;
    result.elements[10] = 1.0f;
    result.elements[15] = 1.0f;
    return result;
}

KENZINE_INLINE Mat4 mat4_mul(Mat4 m0, Mat4 m1)
{
    Mat4 result = mat4_identity();

    const f32* m0_ptr = m0.elements;
    const f32* m1_ptr = m1.elements;
    f32* result_ptr = result.elements;

    for (i32 i = 0; i < 4; ++i)
    {
        for (i32 j = 0; j < 4; ++j)
        {
            *result_ptr = m0_ptr[0] * m1_ptr[0 * 4 + j] +
                          m0_ptr[1] * m1_ptr[1 * 4 + j] +
                          m0_ptr[2] * m1_ptr[2 * 4 + j] +
                          m0_ptr[3] * m1_ptr[3 * 4 + j];
            result_ptr++;
        }

        m0_ptr += 4;
    }

    return result;
}

KENZINE_INLINE Mat4 mat4_proj_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    Mat4 result = mat4_identity();
    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near - far);

    result.elements[0] = -2.0f * lr;
    result.elements[5] = -2.0f * bt;
    result.elements[10] = 2.0f * nf;

    result.elements[12] = (left + right) * lr;
    result.elements[13] = (top + bottom) * bt;
    result.elements[14] = (far + near) * nf;
    return result;
}

KENZINE_INLINE Mat4 mat4_proj_perspective(f32 fov, f32 aspect_ratio, f32 near, f32 far)
{
    Mat4 result = {0};
    f32 tan_half_fov = math_tan(fov * 0.5f);

    result.elements[0] = 1.0f / (aspect_ratio * tan_half_fov);
    result.elements[5] = 1.0f / tan_half_fov;
    result.elements[10] = -((far + near) / (near - far));
    result.elements[11] = -1.0f;
    result.elements[14] = -((2.0f * far * near) / (near - far));
    return result;
}

KENZINE_INLINE Mat4 mat4_look_at(Vec3 pos, Vec3 target, Vec3 up)
{
    Mat4 result = {0};
    Vec3 z_axis = vec3_sub(target, pos);
    vec3_normalize(&z_axis);

    Vec3 x_axis = vec3_cross(z_axis, up);
    vec3_normalize(&x_axis);

    Vec3 y_axis = vec3_cross(x_axis, z_axis);

    result.elements[0] = x_axis.x;
    result.elements[1] = y_axis.x;
    result.elements[2] = -z_axis.x;
    result.elements[3] = 0.0f;
    result.elements[4] = x_axis.y;
    result.elements[5] = y_axis.y;
    result.elements[6] = -z_axis.y;
    result.elements[7] = 0.0f;
    result.elements[8] = x_axis.z;
    result.elements[9] = y_axis.z;
    result.elements[10] = -z_axis.z;
    result.elements[11] = 0.0f;
    result.elements[12] = -vec3_dot(x_axis, pos);
    result.elements[13] = -vec3_dot(y_axis, pos);
    result.elements[14] = vec3_dot(z_axis, pos);
    result.elements[15] = 1.0f;
    return result;
}

KENZINE_INLINE Mat4 mat4_transposed(Mat4 m)
{
    Mat4 result = mat4_identity();
    result.elements[0] = m.elements[0];
    result.elements[1] = m.elements[4];
    result.elements[2] = m.elements[8];
    result.elements[3] = m.elements[12];
    result.elements[4] = m.elements[1];
    result.elements[5] = m.elements[5];
    result.elements[6] = m.elements[9];
    result.elements[7] = m.elements[13];
    result.elements[8] = m.elements[2];
    result.elements[9] = m.elements[6];
    result.elements[10] = m.elements[10];
    result.elements[11] = m.elements[14];
    result.elements[12] = m.elements[3];
    result.elements[13] = m.elements[7];
    result.elements[14] = m.elements[11];
    result.elements[15] = m.elements[15];
    return result;
}

KENZINE_INLINE Mat4 mat4_inverse(Mat4 m)
{
    const f32* m_ptr = m.elements;

    f32 t0 = m_ptr[10] * m_ptr[15];
    f32 t1 = m_ptr[14] * m_ptr[11];
    f32 t2 = m_ptr[6] * m_ptr[15];
    f32 t3 = m_ptr[14] * m_ptr[7];
    f32 t4 = m_ptr[6] * m_ptr[11];
    f32 t5 = m_ptr[10] * m_ptr[7];
    f32 t6 = m_ptr[2] * m_ptr[15];
    f32 t7 = m_ptr[14] * m_ptr[3];
    f32 t8 = m_ptr[2] * m_ptr[11];
    f32 t9 = m_ptr[10] * m_ptr[3];
    f32 t10 = m_ptr[2] * m_ptr[7];
    f32 t11 = m_ptr[6] * m_ptr[3];
    f32 t12 = m_ptr[8] * m_ptr[13];
    f32 t13 = m_ptr[12] * m_ptr[9];
    f32 t14 = m_ptr[4] * m_ptr[13];
    f32 t15 = m_ptr[12] * m_ptr[5];
    f32 t16 = m_ptr[4] * m_ptr[9];
    f32 t17 = m_ptr[8] * m_ptr[5];
    f32 t18 = m_ptr[0] * m_ptr[13];
    f32 t19 = m_ptr[12] * m_ptr[1];
    f32 t20 = m_ptr[0] * m_ptr[9];
    f32 t21 = m_ptr[8] * m_ptr[1];
    f32 t22 = m_ptr[0] * m_ptr[5];
    f32 t23 = m_ptr[4] * m_ptr[1];

    Mat4 result = {0};
    f32* result_ptr = result.elements;

    result_ptr[0] = (t0 * m_ptr[5] + t3 * m_ptr[9] + t4 * m_ptr[13]) - (t1 * m_ptr[5] + t2 * m_ptr[9] + t5 * m_ptr[13]);
    result_ptr[1] = (t1 * m_ptr[1] + t6 * m_ptr[9] + t9 * m_ptr[13]) - (t0 * m_ptr[1] + t7 * m_ptr[9] + t8 * m_ptr[13]);
    result_ptr[2] = (t2 * m_ptr[1] + t7 * m_ptr[5] + t10 * m_ptr[13]) - (t3 * m_ptr[1] + t6 * m_ptr[5] + t11 * m_ptr[13]);
    result_ptr[3] = (t5 * m_ptr[1] + t8 * m_ptr[5] + t11 * m_ptr[9]) - (t4 * m_ptr[1] + t9 * m_ptr[5] + t10 * m_ptr[9]);

    f32 d = 1.0f / (m_ptr[0] * result_ptr[0] + m_ptr[4] * result_ptr[1] + m_ptr[8] * result_ptr[2] + m_ptr[12] * result_ptr[3]);

    result_ptr[0] *= d;
    result_ptr[1] *= d;
    result_ptr[2] *= d;
    result_ptr[3] *= d;

    result_ptr[4] = (t1 * m_ptr[4] + t2 * m_ptr[8] + t5 * m_ptr[12]) - (t0 * m_ptr[4] + t3 * m_ptr[8] + t4 * m_ptr[12]);
    result_ptr[4] *= d;

    result_ptr[5] = (t0 * m_ptr[0] + t7 * m_ptr[8] + t8 * m_ptr[12]) - (t1 * m_ptr[0] + t6 * m_ptr[8] + t9 * m_ptr[12]);
    result_ptr[5] *= d;

    result_ptr[6] = (t3 * m_ptr[0] + t6 * m_ptr[4] + t11 * m_ptr[12]) - (t2 * m_ptr[0] + t7 * m_ptr[4] + t10 * m_ptr[12]);
    result_ptr[6] *= d;

    result_ptr[7] = (t4 * m_ptr[0] + t9 * m_ptr[4] + t10 * m_ptr[8]) - (t5 * m_ptr[0] + t8 * m_ptr[4] + t11 * m_ptr[8]);
    result_ptr[7] *= d;

    result_ptr[8] = (t12 * m_ptr[7] + t15 * m_ptr[11] + t16 * m_ptr[15]) - (t13 * m_ptr[7] + t14 * m_ptr[11] + t17 * m_ptr[15]);
    result_ptr[8] *= d;

    result_ptr[9] = (t13 * m_ptr[3] + t18 * m_ptr[11] + t21 * m_ptr[15]) - (t12 * m_ptr[3] + t19 * m_ptr[11] + t20 * m_ptr[15]);
    result_ptr[9] *= d;

    result_ptr[10] = (t14 * m_ptr[3] + t19 * m_ptr[7] + t22 * m_ptr[15]) - (t15 * m_ptr[3] + t18 * m_ptr[7] + t23 * m_ptr[15]);
    result_ptr[10] *= d;

    result_ptr[11] = (t17 * m_ptr[3] + t20 * m_ptr[7] + t23 * m_ptr[11]) - (t16 * m_ptr[3] + t21 * m_ptr[7] + t22 * m_ptr[11]);
    result_ptr[11] *= d;

    result_ptr[12] = (t14 * m_ptr[10] + t17 * m_ptr[14] + t13 * m_ptr[6]) - (t16 * m_ptr[14] + t12 * m_ptr[6] + t15 * m_ptr[10]);
    result_ptr[12] *= d;

    result_ptr[13] = (t20 * m_ptr[14] + t12 * m_ptr[2] + t19 * m_ptr[10]) - (t18 * m_ptr[10] + t21 * m_ptr[14] + t13 * m_ptr[2]);
    result_ptr[13] *= d;

    result_ptr[14] = (t18 * m_ptr[6] + t23 * m_ptr[14] + t15 * m_ptr[2]) - (t22 * m_ptr[14] + t14 * m_ptr[2] + t19 * m_ptr[6]);
    result_ptr[14] *= d;

    result_ptr[15] = (t22 * m_ptr[10] + t16 * m_ptr[2] + t21 * m_ptr[6]) - (t20 * m_ptr[6] + t23 * m_ptr[10] + t17 * m_ptr[2]);
    result_ptr[15] *= d;

    return result;
}

KENZINE_INLINE Mat4 mat4_translation(Vec3 pos)
{
    Mat4 result = mat4_identity();
    result.elements[12] = pos.x;
    result.elements[13] = pos.y;
    result.elements[14] = pos.z;
    return result;
}

KENZINE_INLINE Mat4 mat4_scale(Vec3 scale)
{
    Mat4 result = mat4_identity();
    result.elements[0] = scale.x;
    result.elements[5] = scale.y;
    result.elements[10] = scale.z;
    return result;
}

KENZINE_INLINE Mat4 mat4_euler_x(f32 angle)
{
    Mat4 result = mat4_identity();
    f32 c = math_cos(angle);
    f32 s = math_sin(angle);

    result.elements[5] = c;
    result.elements[6] = s;
    result.elements[9] = -s;
    result.elements[10] = c;
    return result;
}

KENZINE_INLINE Mat4 mat4_euler_y(f32 angle)
{
    Mat4 result = mat4_identity();
    f32 c = math_cos(angle);
    f32 s = math_sin(angle);

    result.elements[0] = c;
    result.elements[2] = -s;
    result.elements[8] = s;
    result.elements[10] = c;
    return result;
}

KENZINE_INLINE Mat4 mat4_euler_z(f32 angle)
{
    Mat4 result = mat4_identity();
    f32 c = math_cos(angle);
    f32 s = math_sin(angle);

    result.elements[0] = c;
    result.elements[1] = s;
    result.elements[4] = -s;
    result.elements[5] = c;
    return result;
}

KENZINE_INLINE Mat4 mat4_euler_rotation(f32 x, f32 y, f32 z)
{
    Mat4 rx = mat4_euler_x(x);
    Mat4 ry = mat4_euler_y(y);
    Mat4 rz = mat4_euler_z(z);

    Mat4 result = mat4_mul(rx, rx);
    result = mat4_mul(result, rz);
    return result;
}

KENZINE_INLINE Vec3 mat4_forward(Mat4 m)
{
    Vec3 forward = {0};
    forward.x = -m.elements[2];
    forward.y = -m.elements[6];
    forward.z = -m.elements[10];
    vec3_normalize(&forward);
    return forward;
}

KENZINE_INLINE Vec3 mat4_backward(Mat4 m)
{
    Vec3 backward = {0};
    backward.x = m.elements[2];
    backward.y = m.elements[6];
    backward.z = m.elements[10];
    vec3_normalize(&backward);
    return backward;
}

KENZINE_INLINE Vec3 mat4_up(Mat4 m)
{
    Vec3 up = {0};
    up.x = m.elements[1];
    up.y = m.elements[5];
    up.z = m.elements[9];
    vec3_normalize(&up);
    return up;
}

KENZINE_INLINE Vec3 mat4_down(Mat4 m)
{
    Vec3 down = {0};
    down.x = -m.elements[1];
    down.y = -m.elements[5];
    down.z = -m.elements[9];
    vec3_normalize(&down);
    return down;
}

KENZINE_INLINE Vec3 mat4_right(Mat4 m)
{
    Vec3 right = {0};
    right.x = m.elements[0];
    right.y = m.elements[4];
    right.z = m.elements[8];
    vec3_normalize(&right);
    return right;
}

KENZINE_INLINE Vec3 mat4_left(Mat4 m)
{
    Vec3 left = {0};
    left.x = -m.elements[0];
    left.y = -m.elements[4];
    left.z = -m.elements[8];
    vec3_normalize(&left);
    return left;
}