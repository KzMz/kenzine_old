#pragma once

#include "defines.h"
#include "renderer/renderer_defines.h"
#include "lib/containers/hash_table.h"

typedef struct ShaderSystemConfig
{
    u16 max_shader_count;
    u8 max_uniform_count;
    u8 max_global_textures;
    u8 max_instance_textures;
} ShaderSystemConfig;

typedef enum ShaderState
{
    SHADER_STATE_NOT_CREATED,
    SHADER_STATE_UNINITIALIZED,
    SHADER_STATE_INITIALIZED
} ShaderState;

typedef struct ShaderUniform
{
    u64 offset;
    u16 location;
    u16 index;
    u16 size;
    u8 set_index;
    ShaderScope scope;
    ShaderUniformType type;
} ShaderUniform;

typedef struct ShaderAttribute
{
    char* name;
    ShaderAttributeType type;
    u32 size;
} ShaderAttribute;

typedef struct Shader
{
    u64 id;
    char* name;

    bool use_instances;
    bool use_locals;

    u64 required_uniform_alignment;
    
    u64 global_uniform_size;
    u64 global_uniform_stride;
    u64 global_uniform_offset;

    u64 instance_uniform_size;
    u64 instance_uniform_stride;

    u64 push_constant_size;
    u64 push_constant_stride;

    Texture** global_textures;
    u8 instance_texture_count;

    ShaderScope bound_scope;
    u64 bound_instance_id;
    u64 bound_uniform_offset;

    HashTable uniform_lookup;
    ShaderUniform* uniforms;

    ShaderAttribute* attributes;

    ShaderState state;

    u8 push_constant_range_count;
    Range push_constant_ranges[32];

    u16 attribute_stride;

    void* internal_data;
} Shader;

bool shader_system_init(void* state, ShaderSystemConfig config); 
void shader_system_shutdown(void);
u64 shader_system_get_state_size(ShaderSystemConfig config);

KENZINE_API bool shader_system_create(const ShaderConfig* config);
KENZINE_API void shader_system_destroy(const char* shader_name);

KENZINE_API u64 shader_system_get_id(const char* shader_name);
KENZINE_API Shader* shader_system_get_by_id(u64 shader_id);
KENZINE_API Shader* shader_system_get(const char* shader_name);

KENZINE_API bool shader_system_use(const char* shader_name);
KENZINE_API bool shader_system_use_by_id(u64 shader_id);

KENZINE_API u16 shader_system_uniform_index(Shader* shader, const char* uniform_name);
KENZINE_API bool shader_system_uniform_set(const char* uniform_name, const void* value);
KENZINE_API bool shader_system_uniform_set_by_id(u16 uniform_index, const void* value);
KENZINE_API bool shader_system_sampler_set(const char* sampler_name, const Texture* texture);
KENZINE_API bool shader_system_sampler_set_by_id(u16 sampler_index, const Texture* texture);

KENZINE_API bool shader_system_apply_global();
KENZINE_API bool shader_system_apply_instance();
KENZINE_API bool shader_system_bind_instance(u64 instance_id);