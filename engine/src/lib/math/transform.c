#include "transform.h"
#include "lib/math/math.h"
#include "lib/math/vec3.h"
#include "lib/math/quat.h"
#include "lib/math/mat4.h"

Transform transform_create(void)
{
    Transform transform = {0};
    transform_set_position_rotation_scale(&transform, vec3_zero(), quat_identity(), vec3_one());
    transform.local = mat4_identity();
    transform.parent = NULL;
    return transform;
}

Transform transform_from_position(Vec3 position)
{
    Transform transform = {0};
    transform_set_position_rotation_scale(&transform, position, quat_identity(), vec3_one());
    transform.local = mat4_identity();
    transform.parent = NULL;
    return transform;
}

Transform transform_from_rotation(Quat rotation)
{
    Transform transform = {0};
    transform_set_position_rotation_scale(&transform, vec3_zero(), rotation, vec3_one());
    transform.local = mat4_identity();
    transform.parent = NULL;
    return transform;
}

Transform transform_from_position_rotation(Vec3 position, Quat rotation)
{
    Transform transform = {0};
    transform_set_position_rotation_scale(&transform, position, rotation, vec3_one());
    transform.local = mat4_identity();
    transform.parent = NULL;
    return transform;
}

Transform transform_from_position_rotation_scale(Vec3 position, Quat rotation, Vec3 scale)
{
    Transform transform = {0};
    transform_set_position_rotation_scale(&transform, position, rotation, scale);
    transform.local = mat4_identity();
    transform.parent = NULL;
    return transform;
}

Transform* transform_get_parent(const Transform* transform)
{
    if (transform != NULL)
    {
        return transform->parent;
    }

    return NULL;
}

void transform_set_parent(Transform* transform, Transform* parent)
{
    if (transform != NULL)
    {
        transform->parent = parent;
    }
}

Vec3 transform_get_position(const Transform* transform)
{
    if (transform != NULL)
    {
        return transform->position;
    }

    return vec3_zero();
}

void transform_set_position(Transform* transform, Vec3 position)
{
    if (transform != NULL)
    {
        transform->position = position;
        transform->is_dirty = true;
    }
}

void transform_translate(Transform* transform, Vec3 translation)
{
    if (transform != NULL)
    {
        transform->position = vec3_add(transform->position, translation);
        transform->is_dirty = true;
    }
}

Quat transform_get_rotation(const Transform* transform)
{
    if (transform != NULL)
    {
        return transform->rotation;
    }

    return quat_identity();
}

void transform_set_rotation(Transform* transform, Quat rotation)
{
    if (transform != NULL)
    {
        transform->rotation = rotation;
        transform->is_dirty = true;
    }
}

void transform_rotate(Transform* transform, Quat rotation)
{
    if (transform != NULL)
    {
        transform->rotation = quat_mul(transform->rotation, rotation);
        transform->is_dirty = true;
    }
}

Vec3 transform_get_scale(const Transform* transform)
{
    if (transform != NULL)
    {
        return transform->scale;
    }

    return vec3_one();
}

void transform_set_scale(Transform* transform, Vec3 scale)
{
    if (transform != NULL)
    {
        transform->scale = scale;
        transform->is_dirty = true;
    }
}

void transform_scale(Transform* transform, Vec3 scale)
{
    if (transform != NULL)
    {
        transform->scale = vec3_mul(transform->scale, scale);
        transform->is_dirty = true;
    }
}

void transform_set_position_rotation(Transform* transform, Vec3 position, Quat rotation)
{
    if (transform != NULL)
    {
        transform->position = position;
        transform->rotation = rotation;
        transform->is_dirty = true;
    }
}

void transform_set_position_rotation_scale(Transform* transform, Vec3 position, Quat rotation, Vec3 scale)
{
    if (transform != NULL)
    {
        transform->position = position;
        transform->rotation = rotation;
        transform->scale = scale;
        transform->is_dirty = true;
    }
}

void transform_translate_rotate(Transform* transform, Vec3 translation, Quat rotation)
{
    if (transform != NULL)
    {
        transform->position = vec3_add(transform->position, translation);
        transform->rotation = quat_mul(transform->rotation, rotation);
        transform->is_dirty = true;
    }
}

Mat4 transform_get_local(Transform* transform)
{
    if (transform != NULL)
    {
        if (transform->is_dirty)
        {
            Mat4 translation = mat4_mul(quat_to_mat4(transform->rotation), mat4_translation(transform->position));
            translation = mat4_mul(mat4_scale(transform->scale), translation);
            transform->local = translation;
            transform->is_dirty = false;
        }

        return transform->local;
    }

    return mat4_identity();
}

Mat4 transform_get_world(Transform* transform)
{
    if (transform != NULL)
    {
        Mat4 world = transform_get_local(transform);
        Transform* parent = transform_get_parent(transform);
        while (parent != NULL)
        {
            Mat4 parent_world = transform_get_local(parent);
            world = mat4_mul(world, parent_world);
            parent = transform_get_parent(parent);
        }

        return world;
    }

    return mat4_identity();
}