#pragma once

#include "defines.h"
#include "resources/resource_defines.h"

#define DEFAULT_MATERIAL_NAME "default"

typedef struct MaterialSystemConfig
{
    u32 max_materials;
} MaterialSystemConfig;

typedef struct MaterialConfig
{
    char name[MATERIAL_NAME_MAX_LENGTH];
    bool auto_release;
    Vec4 diffuse_color;
    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
} MaterialConfig;

bool material_system_init(void* state, MaterialSystemConfig config);
void material_system_shutdown(void);
u64 material_system_get_state_size(MaterialSystemConfig config);

Material* material_system_acquire(const char* name);
Material* material_system_acquire_from_config(MaterialConfig config);
void material_system_release(const char* name);

Material* material_system_get_default(void);