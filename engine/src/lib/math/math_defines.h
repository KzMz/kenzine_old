#pragma once

#include "defines.h"

typedef union Vec2_u 
{
    f32 elements[2];
    struct 
    {
        union 
        {
            f32 x, r, s, u;
        };
        union 
        {
            f32 y, g, t, v;
        };
    };
} Vec2;

typedef union Vec3_u 
{
    f32 elements[3];
    struct 
    {
        union 
        {
            f32 x, r, s, u;
        };
        union 
        {
            f32 y, g, t, v;
        };
        union 
        {
            f32 z, b, p, w;
        };
    };
} Vec3;

typedef union Vec4_u
{
    f32 elements[4];
    union 
    {
        struct 
        {
            union 
            {
                f32 x, r, s;
            };
            union 
            {
                f32 y, g, t;
            };
            union 
            {
                f32 z, b, p;
            };
            union 
            {
                f32 w, a, q;
            };
        };
    };
} Vec4;

typedef Vec4 Quat;

typedef union Mat4_u
{
    f32 elements[4 * 4];
} Mat4;

typedef struct Vertex2d
{
    Vec2 position;
    Vec2 texcoord;
} Vertex2d;

typedef struct Vertex3d 
{
    Vec3 position;
    Vec3 normal;
    Vec2 texcoord;
    Vec4 color;
    Vec4 tangent;
} Vertex3d;

typedef struct Transform
{
    Vec3 position;
    Quat rotation;
    Vec3 scale;

    bool is_dirty;

    Mat4 local;

    struct Transform* parent;
} Transform;