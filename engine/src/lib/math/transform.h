#pragma once

#include "defines.h"
#include "lib/math/math_defines.h"

KENZINE_API Transform transform_create(void);
KENZINE_API Transform transform_from_position(Vec3 position);
KENZINE_API Transform transform_from_rotation(Quat rotation);
KENZINE_API Transform transform_from_position_rotation(Vec3 position, Quat rotation);
KENZINE_API Transform transform_from_position_rotation_scale(Vec3 position, Quat rotation, Vec3 scale);

KENZINE_API Transform* transform_get_parent(const Transform* transform);
KENZINE_API void transform_set_parent(Transform* transform, Transform* parent);

KENZINE_API Vec3 transform_get_position(const Transform* transform);
KENZINE_API void transform_set_position(Transform* transform, Vec3 position);

KENZINE_API void transform_translate(Transform* transform, Vec3 translation);

KENZINE_API Quat transform_get_rotation(const Transform* transform);
KENZINE_API void transform_set_rotation(Transform* transform, Quat rotation);

KENZINE_API void transform_rotate(Transform* transform, Quat rotation);

KENZINE_API Vec3 transform_get_scale(const Transform* transform);
KENZINE_API void transform_set_scale(Transform* transform, Vec3 scale);

KENZINE_API void transform_scale(Transform* transform, Vec3 scale);

KENZINE_API void transform_set_position_rotation(Transform* transform, Vec3 position, Quat rotation);
KENZINE_API void transform_set_position_rotation_scale(Transform* transform, Vec3 position, Quat rotation, Vec3 scale);

KENZINE_API void transform_translate_rotate(Transform* transform, Vec3 translation, Quat rotation);

KENZINE_API Mat4 transform_get_local(Transform* transform);
KENZINE_API Mat4 transform_get_world(Transform* transform);