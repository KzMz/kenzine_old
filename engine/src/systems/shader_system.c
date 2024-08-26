#include "shader_system.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"

#include "lib/containers/dyn_array.h"
#include "renderer/renderer_frontend.h"

#include "systems/texture_system.h"
#include <stddef.h>

typedef struct ShaderSystemState
{
    ShaderSystemConfig config;
    HashTable lookup;
    u64 current_shader_id;
    Shader* shaders;
} ShaderSystemState;

static ShaderSystemState* shader_system_state = NULL;

bool add_attribute(Shader* shader, const ShaderAttributeConfig* config);
bool add_sampler(Shader* shader, const ShaderUniformConfig* config);
bool add_uniform(Shader* shader, const ShaderUniformConfig* config);

u64 get_shader_id(const char* shader_name);
u64 new_shader_id(void);

bool uniform_add(Shader* shader, const char* uniform_name, u64 size, ShaderUniformType type, ShaderScope scope, u64 set_location, bool is_sampler);
bool uniform_name_valid(Shader* shader, const char* uniform_name);
bool shader_uniform_add_state_valid(Shader* shader);
void shader_destroy(Shader* shader);

bool shader_system_init(void* state, ShaderSystemConfig config)
{
    if (config.max_shader_count < 512)
    {
        if (config.max_shader_count == 0)
        {
            log_error("Shader system config is invalid. Max shader count must be greater than 0.");
            return false;
        } 
        else 
        {
            log_warning("Shader system config is invalid. Max shader count should be greater than 512.");
        }
    }

    shader_system_state = (ShaderSystemState*) state;
    u64 addr = (u64) state;
    shader_system_state->config = config;
    shader_system_state->current_shader_id = INVALID_ID;
    shader_system_state->shaders = (Shader*) (addr + sizeof(ShaderSystemState));
    hashtable_create(u32, config.max_shader_count, false, &shader_system_state->lookup);

    for (u32 i = 0; i < config.max_shader_count; ++i)
    {
        shader_system_state->shaders[i].id = INVALID_ID;
    }

    u32 invalid_fill_id = INVALID_ID;
    hashtable_fill_with_value(&shader_system_state->lookup, &invalid_fill_id);

    return true;
}

void shader_system_shutdown(void)
{
    if (shader_system_state == NULL)
    {
        return;
    }

    for (u32 i = 0; i < shader_system_state->config.max_shader_count; ++i)
    {
        if (shader_system_state->shaders[i].id != INVALID_ID)
        {
            shader_destroy(&shader_system_state->shaders[i]);
        }
    }

    hashtable_destroy(&shader_system_state->lookup);
    memory_zero(shader_system_state, sizeof(ShaderSystemState));
}

u64 shader_system_get_state_size(ShaderSystemConfig config)
{
    return sizeof(ShaderSystemState) + sizeof(Shader) * config.max_shader_count; // TODO: see if there's the need to move hashtable memory here
}

bool shader_system_create(const ShaderConfig* config)
{
    u64 id = new_shader_id();
    Shader* out_shader = &shader_system_state->shaders[id];
    memory_zero(out_shader, sizeof(Shader));
    out_shader->id = id;

    if (out_shader->id == INVALID_ID)
    {
        log_error("Failed to create shader. No more shader slots available.");
        return false;
    }

    out_shader->state = SHADER_STATE_NOT_CREATED;
    out_shader->name = string_clone(config->name);
    out_shader->use_instances = config->use_instances;
    out_shader->use_locals = config->use_local;
    out_shader->push_constant_range_count = 0;
    memory_zero(out_shader->push_constant_ranges, sizeof(Range) * 32);
    out_shader->bound_instance_id = INVALID_ID;
    out_shader->attribute_stride = 0;

    out_shader->global_textures = dynarray_create(Texture*);
    out_shader->uniforms = dynarray_create(ShaderUniform);
    out_shader->attributes = dynarray_create(ShaderAttribute);

    u64 element_count = 1024;
    hashtable_create(u16, element_count, false, &out_shader->uniform_lookup);

    u64 invalid = INVALID_ID;
    hashtable_fill_with_value(&out_shader->uniform_lookup, &invalid);

    out_shader->global_uniform_size = 0;
    out_shader->instance_uniform_size = 0;

    out_shader->push_constant_stride = 128;
    out_shader->push_constant_size = 0;

    u8 renderpass_id = INVALID_ID_U8;
    if (!renderer_renderpass_id(config->renderpass_name, &renderpass_id))
    {
        log_error("Failed to get renderpass id for shader %s.", config->name);
        return false;
    }

    if (!renderer_shader_create(out_shader, renderpass_id, config->stage_count, config->stage_files, config->stages))
    {
        log_error("Failed to create shader %s.", config->name);
        return false;
    }

    out_shader->state = SHADER_STATE_UNINITIALIZED;

    for (u32 i = 0; i < config->attribute_count; ++i)
    {
        add_attribute(out_shader, &config->attributes[i]);
    }

    for (u32 i = 0; i < config->uniform_count; ++i)
    {
        if (config->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER)
        {
            add_sampler(out_shader, &config->uniforms[i]);
        }
        else 
        {
            add_uniform(out_shader, &config->uniforms[i]);
        }
    }

    if (!renderer_shader_init(out_shader))
    {
        log_error("Failed to initialize shader %s.", config->name);
        return false;
    }

    hashtable_set(&shader_system_state->lookup, config->name, &out_shader->id);
    if (out_shader->id == INVALID_ID)
    {
        renderer_shader_destroy(out_shader);
        return false;
    }

    return true;
}

u64 shader_system_get_id(const char* shader_name)
{
    return get_shader_id(shader_name);
}

Shader* shader_system_get_by_id(u64 shader_id)
{
    if (shader_id >= shader_system_state->config.max_shader_count || shader_system_state->shaders[shader_id].id == INVALID_ID)
    {
        return NULL;
    }

    return &shader_system_state->shaders[shader_id];
}

Shader* shader_system_get(const char* shader_name)
{
    u64 id = get_shader_id(shader_name);
    if (id == INVALID_ID)
    {
        return NULL;
    }
    return shader_system_get_by_id(id);
}

void shader_destroy(Shader* shader)
{
    renderer_shader_destroy(shader);
    shader->state = SHADER_STATE_NOT_CREATED;

    if (shader->name != NULL)
    {
        u32 length = string_length(shader->name);
        memory_free(shader->name, length + 1, MEMORY_TAG_STRING);
    }

    shader->name = NULL;
}

void shader_system_destroy(const char* shader_name)
{
    u64 shader_id = get_shader_id(shader_name);
    if (shader_id == INVALID_ID)
    {
        return;
    }

    Shader* shader = shader_system_get_by_id(shader_id);
    if (shader == NULL)
    {
        return;
    }
    shader_destroy(shader);
}

bool shader_system_use(const char* shader_name)
{
    u64 next_shader_id = get_shader_id(shader_name);
    if (next_shader_id == INVALID_ID)
    {
        return false;
    }

    return shader_system_use_by_id(next_shader_id);
}

bool shader_system_use_by_id(u64 shader_id)
{
    if (shader_system_state->current_shader_id != shader_id)
    {
        Shader* shader = shader_system_get_by_id(shader_id);
        shader_system_state->current_shader_id = shader_id;
        if (!renderer_shader_use(shader))
        {
            log_error("Failed to use shader %s.", shader->name);
            return false;
        }

        if (!renderer_shader_bind_globals(shader))
        {
            log_error("Failed to bind globals for shader %s.", shader->name);
            return false;
        }
    }

    return true;
}

u16 shader_system_uniform_index(Shader* shader, const char* uniform_name)
{
    if (shader == NULL || shader->id == INVALID_ID)
    {
        log_error("shader_system_uniform_index: Shader is invalid.");
        return INVALID_ID_U16;
    }

    u16 index = INVALID_ID_U16;
    hashtable_get(&shader->uniform_lookup, uniform_name, &index);
    if (index == INVALID_ID_U16)
    {
        log_error("shader_system_uniform_index: Uniform %s not found in shader %s.", uniform_name, shader->name);
        return INVALID_ID_U16;
    }

    return shader->uniforms[index].index;
}

bool shader_system_uniform_set(const char* uniform_name, const void* value)
{
    if (shader_system_state->current_shader_id == INVALID_ID)
    {
        log_error("shader_system_uniform_set: No shader is currently bound.");
        return false;
    }

    Shader* shader = &shader_system_state->shaders[shader_system_state->current_shader_id];
    u16 index = shader_system_uniform_index(shader, uniform_name);
    return shader_system_uniform_set_by_id(index, value);
}

bool shader_system_sampler_set(const char* sampler_name, const Texture* texture)
{
    return shader_system_uniform_set(sampler_name, texture);
}

bool shader_system_uniform_set_by_id(u16 uniform_index, const void* value)
{
    Shader* shader = &shader_system_state->shaders[shader_system_state->current_shader_id];
    ShaderUniform* uniform = &shader->uniforms[uniform_index];
    if (shader->bound_scope != uniform->scope)
    {
        if (uniform->scope == SHADER_SCOPE_GLOBAL)
        {
            renderer_shader_bind_globals(shader);
        }
        else if (uniform->scope == SHADER_SCOPE_INSTANCE)
        {
            renderer_shader_bind_instance(shader, shader->bound_instance_id);
        }

        shader->bound_scope = uniform->scope;
    }
    return renderer_shader_set_uniform(shader, uniform, value);
}

bool shader_system_sampler_set_by_id(u16 sampler_index, const Texture* texture)
{
    return shader_system_uniform_set_by_id(sampler_index, texture);
}

bool shader_system_apply_global()
{
    return renderer_shader_apply_globals(&shader_system_state->shaders[shader_system_state->current_shader_id]);
}

bool shader_system_apply_instance()
{
    return renderer_shader_apply_instance(&shader_system_state->shaders[shader_system_state->current_shader_id]);
}

bool shader_system_bind_instance(u64 instance_id)
{
    Shader* shader = &shader_system_state->shaders[shader_system_state->current_shader_id];
    shader->bound_instance_id = instance_id;

    return renderer_shader_bind_instance(shader, instance_id);
}

bool add_attribute(Shader* shader, const ShaderAttributeConfig* config)
{
    u32 size = 0;
    switch (config->type)
    {
        case SHADER_ATTRIB_TYPE_INT8:
        case SHADER_ATTRIB_TYPE_UINT8:
            size = 1;
            break;
        case SHADER_ATTRIB_TYPE_INT16:
        case SHADER_ATTRIB_TYPE_UINT16:
            size = 2;
            break;
        case SHADER_ATTRIB_TYPE_INT32:
        case SHADER_ATTRIB_TYPE_UINT32:
        case SHADER_ATTRIB_TYPE_FLOAT32:
            size = 4;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_2:
            size = 8;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_3:
            size = 12;
            break;
        case SHADER_ATTRIB_TYPE_FLOAT32_4:
            size = 16;
            break;
        default:
            log_error("add_attribute: Invalid attribute type. Defaulting to 4 bytes.");
            size = 4;
            break;
    }

    shader->attribute_stride += size;

    ShaderAttribute attribute = 
    {
        .name = string_clone(config->name),
        .size = size,
        .type = config->type
    };
    dynarray_push(shader->attributes, attribute);
    return true;
}

bool add_sampler(Shader* shader, const ShaderUniformConfig* config)
{
    if (config->scope == SHADER_SCOPE_INSTANCE && !shader->use_instances)
    {
        log_error("add_sampler: Shader %s does not support instances.", shader->name);
        return false;
    }
    if (config->scope == SHADER_SCOPE_LOCAL)
    {
        log_error("add_sampler: Local samplers are not supported.");
        return false;
    }

    if (!uniform_name_valid(shader, config->name) || !shader_uniform_add_state_valid(shader))
    {
        return false;
    }

    u64 location = 0;
    if (config->scope == SHADER_SCOPE_GLOBAL)
    {
        u32 global_texture_count = dynarray_length(shader->global_textures);
        if (global_texture_count + 1 > shader_system_state->config.max_global_textures)
        {
            log_error("add_sampler: Shader %s has reached the maximum number of global textures.", shader->name);
            return false;
        }

        location = global_texture_count;
        dynarray_push(shader->global_textures, texture_system_get_default());
    }
    else 
    {
        if (shader->instance_texture_count + 1 > shader_system_state->config.max_instance_textures)
        {
            log_error("add_sampler: Shader %s has reached the maximum number of instance textures.", shader->name);
            return false;
        }

        location = shader->instance_texture_count;
        shader->instance_texture_count++;
    }

    if (!uniform_add(shader, config->name, 0, config->type, config->scope, location, true))
    {
        log_error("add_sampler: Failed to add uniform %s to shader %s.", config->name, shader->name);
        return false;
    }

    return true;
}

bool add_uniform(Shader* shader, const ShaderUniformConfig* config)
{
    if (!shader_uniform_add_state_valid(shader) || !uniform_name_valid(shader, config->name))
    {
        return false;
    }

    return uniform_add(shader, config->name, config->size, config->type, config->scope, 0, false);
}

u64 get_shader_id(const char* shader_name)
{
    u64 shader_id = INVALID_ID;
    hashtable_get(&shader_system_state->lookup, shader_name, &shader_id);
    if (shader_id == INVALID_ID)
    {
        log_error("get_shader_id: Shader %s not found.", shader_name);
        return INVALID_ID;
    }

    return shader_id;
}

u64 new_shader_id(void)
{
    for (u64 i = 0; i < shader_system_state->config.max_shader_count; ++i)
    {
        if (shader_system_state->shaders[i].id == INVALID_ID)
        {
            return i;
        }
    }

    return INVALID_ID;
}

bool uniform_add(Shader* shader, const char* uniform_name, u64 size, ShaderUniformType type, ShaderScope scope, u64 set_location, bool is_sampler)
{
    u32 uniform_count = dynarray_length(shader->uniforms);
    if (uniform_count + 1 > shader_system_state->config.max_uniform_count)
    {
        log_error("uniform_add: Shader %s has reached the maximum number of uniforms.", shader->name);
        return false;
    }

    ShaderUniform uniform = 
    {
        .index = uniform_count,
        .scope = scope,
        .type = type
    };
    bool is_global = (scope == SHADER_SCOPE_GLOBAL);
    if (is_sampler)
    {
        uniform.location = set_location;
    }
    else 
    {
        uniform.location = uniform.index;
    }

    if (scope != SHADER_SCOPE_LOCAL)
    {
        uniform.set_index = (u32) scope;
        uniform.offset = is_sampler ? 0 : is_global ? shader->global_uniform_size : shader->instance_uniform_size;
        uniform.size = is_sampler ? 0 : size;
    }
    else 
    {
        if (uniform.scope == SHADER_SCOPE_LOCAL && !shader->use_locals)
        {
            log_error("uniform_add: Shader %s does not support local uniforms.", shader->name);
            return false;
        }

        uniform.set_index = INVALID_ID_U8;
        Range range = get_aligned_range(shader->push_constant_size, size, 4); // required by Vulkan spec
        uniform.offset = range.offset;
        uniform.size = range.size;

        shader->push_constant_ranges[shader->push_constant_range_count] = range;
        shader->push_constant_range_count++;

        shader->push_constant_size += range.size;
    }

    hashtable_set(&shader->uniform_lookup, uniform_name, &uniform.index);
    dynarray_push(shader->uniforms, uniform);
    
    if (!is_sampler)
    {
        if (uniform.scope == SHADER_SCOPE_GLOBAL)
        {
            shader->global_uniform_size += uniform.size;
        }
        else if (uniform.scope == SHADER_SCOPE_INSTANCE)
        {
            shader->instance_uniform_size += uniform.size;
        }
    }

    return true;
}

bool uniform_name_valid(Shader* shader, const char* uniform_name)
{
    if (uniform_name == NULL || string_length(uniform_name) == 0)
    {
        log_error("uniform_name_valid: Uniform name is invalid.");
        return false;
    }

    u16 location = INVALID_ID_U16;
    hashtable_get(&shader->uniform_lookup, uniform_name, &location);

    if (location != INVALID_ID_U16)
    {
        log_error("uniform_name_valid: Uniform %s already exists in shader %s.", uniform_name, shader->name);
        return false;
    }

    return true;
}

bool shader_uniform_add_state_valid(Shader* shader)
{
    if (shader->state != SHADER_STATE_UNINITIALIZED)
    {
        log_error("shader_uniform_add_state_valid: Shader %s is not in an uninitialized state.", shader->name);
        return false;
    }

    return true;
}