#pragma once

#include "defines.h"
#include "resources/resource_defines.h"

#define DEFAULT_MATERIAL_NAME "default"

typedef struct MaterialSystemConfig
{
    u32 max_materials;
} MaterialSystemConfig;

bool material_system_init(void* state, MaterialSystemConfig config);
void material_system_shutdown(void);
u64 material_system_get_state_size(MaterialSystemConfig config);

Material* material_system_acquire(const char* name);
Material* material_system_acquire_from_config(MaterialResourceData config);
void material_system_release(const char* name);

Material* material_system_get_default(void);

bool material_system_apply_global(u64 shader_id, const Mat4* projection, const Mat4* view, const Vec4* ambient_color);
bool material_system_apply_instance(Material* material);
bool material_system_apply_local(Material* material, const Mat4* model);