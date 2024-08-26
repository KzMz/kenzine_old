#include "geometry_utils.h"
#include "lib/math/vec3.h"
#include "lib/math/vec4.h"

void geometry_generate_normals(u32 vertex_count, Vertex3d* vertices, u32 index_count, u32* indices)
{
    for (u32 i = 0; i < index_count; i += 3)
    {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        Vec3 edge1 = vec3_sub(vertices[i1].position, vertices[i0].position);
        Vec3 edge2 = vec3_sub(vertices[i2].position, vertices[i0].position);
        Vec3 normal = vec3_cross(edge1, edge2);
        vec3_normalize(&normal);

        vertices[i0].normal = normal;
        vertices[i1].normal = normal;
        vertices[i2].normal = normal;
    }
}

void geometry_generate_tangents(u32 vertex_count, Vertex3d* vertices, u32 index_count, u32* indices)
{
    for (u32 i = 0; i < index_count; i += 3)
    {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        Vec3 edge1 = vec3_sub(vertices[i1].position, vertices[i0].position);
        Vec3 edge2 = vec3_sub(vertices[i2].position, vertices[i0].position);

        f32 delta_u1 = vertices[i1].texcoord.x - vertices[i0].texcoord.x;
        f32 delta_v1 = vertices[i1].texcoord.y - vertices[i0].texcoord.y;

        f32 delta_u2 = vertices[i2].texcoord.x - vertices[i0].texcoord.x;
        f32 delta_v2 = vertices[i2].texcoord.y - vertices[i0].texcoord.y;

        f32 dividend = (delta_u1 * delta_v2 - delta_u2 * delta_v1);
        f32 f = 1.0f / dividend;

        Vec3 tangent = (Vec3) 
        {
            f * (delta_v2 * edge1.x - delta_v1 * edge2.x),
            f * (delta_v2 * edge1.y - delta_v1 * edge2.y),
            f * (delta_v2 * edge1.z - delta_v1 * edge2.z)
        };
        kz_assert(vec3_length(tangent) > 0.0f);
        vec3_normalize(&tangent);

        f32 sx = delta_u1, sy = delta_u2;
        f32 tx = delta_v1, ty = delta_v2;
        f32 handedness = (tx * sy - ty * sx) < 0.0f ? -1.0f : 1.0f;
        Vec4 t4 = vec4_from_vec3(tangent, handedness);
        vertices[i0].tangent = t4;
        vertices[i1].tangent = t4;
        vertices[i2].tangent = t4;
    }
}