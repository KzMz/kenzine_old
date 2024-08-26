#include "material_system.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/containers/hash_table.h"
#include "lib/containers/dyn_array.h"
#include "lib/math/math_defines.h"
#include "lib/math/vec4.h"
#include "renderer/renderer_frontend.h"
#include "systems/texture_system.h"
#include "lib/string.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

typedef struct MaterialShaderUniformLocations
{
    u16 projection;
    u16 view;
    u16 diffuse_color;
    u16 diffuse_texture;
    u16 model;
} MaterialShaderUniformLocations;

typedef struct UIShaderUniformLocations
{
    u16 projection;
    u16 view;
    u16 diffuse_color;
    u16 diffuse_texture;
    u16 model;
} UIShaderUniformLocations;

typedef struct MaterialSystemState
{
    MaterialSystemConfig config;

    Material default_material;

    Material* materials;
    HashTable material_table;

    MaterialShaderUniformLocations material_locations;
    u32 material_shader_id;

    UIShaderUniformLocations ui_locations;
    u32 ui_shader_id;
} MaterialSystemState;

typedef struct MaterialReference
{
    u64 reference_count;
    u32 handle;
    bool auto_release;
} MaterialReference;

static MaterialSystemState* material_system_state = NULL;

bool create_default_material(MaterialSystemState* state);
bool load_material(MaterialResourceData config, Material* out_material);
void destroy_material(Material* material);

bool material_system_init(void* state, MaterialSystemConfig config)
{
    if (config.max_materials == 0)
    {
        log_error("Material system config is invalid. Max materials must be greater than 0.");
        return false;
    }

    material_system_state = (MaterialSystemState*) state;
    material_system_state->config = config;

    material_system_state->material_shader_id = INVALID_ID;
    material_system_state->material_locations.diffuse_color = INVALID_ID_U16;
    material_system_state->material_locations.diffuse_texture = INVALID_ID_U16;

    material_system_state->ui_shader_id = INVALID_ID;
    material_system_state->ui_locations.diffuse_color = INVALID_ID_U16;
    material_system_state->ui_locations.diffuse_texture = INVALID_ID_U16;

    material_system_state->materials = state + sizeof(MaterialSystemState);
    hashtable_create(MaterialReference, config.max_materials, false, &material_system_state->material_table);

    MaterialReference invalid_ref;
    invalid_ref.reference_count = 0;
    invalid_ref.auto_release = false;
    invalid_ref.handle = INVALID_ID;
    hashtable_fill_with_value(&material_system_state->material_table, &invalid_ref);

    for (u32 i = 0; i < config.max_materials; ++i)
    {
        material_system_state->materials[i].generation = INVALID_ID;
        material_system_state->materials[i].id = INVALID_ID;
        material_system_state->materials[i].internal_id = INVALID_ID;
    }

    if (!create_default_material(material_system_state))
    {
        log_fatal("Failed to create default material.");
        return false;
    }

    return true;
}

void material_system_shutdown(void)
{
    if (material_system_state == NULL)
    {
        return;
    }

    for (u32 i = 0; i < material_system_state->config.max_materials; ++i)
    {
        if (material_system_state->materials[i].id != INVALID_ID)
        {
            destroy_material(&material_system_state->materials[i]);
        }
    }

    destroy_material(&material_system_state->default_material);

    memory_zero(material_system_state->materials, sizeof(Material) * material_system_state->config.max_materials);

    hashtable_destroy(&material_system_state->material_table);
    memory_zero(material_system_state, sizeof(MaterialSystemState));
    material_system_state = NULL;
}

u64 material_system_get_state_size(MaterialSystemConfig config)
{
    return sizeof(MaterialSystemState) + (sizeof(Material) * config.max_materials);
}

Material* material_system_acquire(const char* name)
{
    Resource resource = {0};
    if (!resource_system_load(name, RESOURCE_TYPE_MATERIAL, &resource))
    {
        log_error("Failed to load material: %s", name);
        return NULL;
    }

    Material* material = NULL;
    if (resource.data != NULL)
    {
        MaterialResourceData* config = (MaterialResourceData*) resource.data;
        material = material_system_acquire_from_config(*config);
    }

    resource_system_unload(&resource);

    if (material == NULL)
    {
        log_error("Failed to acquire material: %s", name);
        return NULL;
    }

    return material;
}

Material* material_system_acquire_from_config(MaterialResourceData config)
{
    if (string_equals_nocase(config.name, DEFAULT_MATERIAL_NAME))
    {
        return &material_system_state->default_material;
    }

    MaterialReference ref;
    hashtable_get(&material_system_state->material_table, config.name, &ref);

    if (ref.reference_count == 0)
    {
        ref.auto_release = config.auto_release;
    }

    ref.reference_count++;

    if (ref.handle == INVALID_ID)
    {
        Material* material = NULL;
        for (u32 i = 0; i < material_system_state->config.max_materials; ++i)
        {
            if (material_system_state->materials[i].id == INVALID_ID)
            {
                ref.handle = i;
                material = &material_system_state->materials[i];
                break;
            }
        }

        if (material == NULL || ref.handle == INVALID_ID)
        {
            log_error("Failed to acquire material: %s. No more material slots available.", config.name);
            return NULL;
        }

        if (!load_material(config, material))
        {
            log_error("Failed to load material: %s", config.name);
            return NULL;
        }

        Shader* shader = shader_system_get_by_id(material->shader_id);
        if (material_system_state->material_shader_id == INVALID_ID && string_equals(config.shader_name, BUILTIN_SHADER_NAME_MATERIAL))
        {
            material_system_state->material_shader_id = shader->id;
            material_system_state->material_locations.projection = shader_system_uniform_index(shader, "projection");
            material_system_state->material_locations.view = shader_system_uniform_index(shader, "view");
            material_system_state->material_locations.diffuse_color = shader_system_uniform_index(shader, "diffuse_color");
            material_system_state->material_locations.diffuse_texture = shader_system_uniform_index(shader, "diffuse_texture");
            material_system_state->material_locations.model = shader_system_uniform_index(shader, "model");
        }
        else if (material_system_state->ui_shader_id == INVALID_ID && string_equals(config.shader_name, BUILTIN_SHADER_NAME_UI))
        {
            material_system_state->ui_shader_id = shader->id;
            material_system_state->ui_locations.projection = shader_system_uniform_index(shader, "projection");
            material_system_state->ui_locations.view = shader_system_uniform_index(shader, "view");
            material_system_state->ui_locations.diffuse_color = shader_system_uniform_index(shader, "diffuse_color");
            material_system_state->ui_locations.diffuse_texture = shader_system_uniform_index(shader, "diffuse_texture");
            material_system_state->ui_locations.model = shader_system_uniform_index(shader, "model");
        }

        if (material->generation == INVALID_ID)
        {
            material->generation = 0;
        }
        else 
        {
            material->generation++;
        }

        material->id = ref.handle;
        log_trace("Material acquired: %s", config.name);
    }

    hashtable_set(&material_system_state->material_table, config.name, &ref);
    return &material_system_state->materials[ref.handle];
}

void material_system_release(const char* name)
{
    if (string_equals_nocase(name, DEFAULT_MATERIAL_NAME))
    {
        return;
    }

    MaterialReference ref;
    hashtable_get(&material_system_state->material_table, name, &ref);

    if (ref.reference_count == 0)
    {
        log_warning("Material: %s is not acquired.", name);
        return;
    }

    ref.reference_count--;

    if (ref.reference_count == 0 && ref.auto_release)
    {
        Material* material = &material_system_state->materials[ref.handle];
        destroy_material(material);

        ref.handle = INVALID_ID;
        ref.auto_release = false;
        log_trace("Material released: %s", name);
    }

    hashtable_set(&material_system_state->material_table, name, &ref);
}

Material* material_system_get_default(void)
{
    if (material_system_state == NULL)
    {
        log_error("Material system is not initialized.");
        return NULL;
    }

    return &material_system_state->default_material;
}

bool load_material(MaterialResourceData config, Material* out_material)
{
    memory_zero(out_material, sizeof(Material));

    string_copy_n(out_material->name, config.name, MATERIAL_NAME_MAX_LENGTH);
    out_material->shader_id = shader_system_get_id(config.shader_name);
    out_material->diffuse_color = config.diffuse_color;

    if (string_length(config.diffuse_map_name) > 0)
    {
        out_material->diffuse_map.usage = TEXTURE_USE_DIFFUSE;
        out_material->diffuse_map.texture = texture_system_acquire(config.diffuse_map_name, true);
        if (!out_material->diffuse_map.texture)
        {
            log_warning("Failed to acquire texture: %s. Using default", config.diffuse_map_name);
            out_material->diffuse_map.texture = texture_system_get_default();
        }
    }
    else 
    {
        out_material->diffuse_map.usage = TEXTURE_USE_UNKNOWN;
        out_material->diffuse_map.texture = NULL;
    }

    // other maps here

    Shader* shader = shader_system_get(config.shader_name);
    if (shader == NULL)
    {
        log_error("Failed to get shader: %s", config.shader_name);
        return false;
    }

    if (!renderer_shader_acquire_instance_resources(shader, &out_material->internal_id))
    {
        log_error("Failed to acquire instance resources for material: %s", config.name);
        return false;
    }

    return true;
}

void destroy_material(Material* material)
{
    if (material->diffuse_map.texture)
    {
        texture_system_release(material->diffuse_map.texture->name);
    }

    if (material->shader_id != INVALID_ID && material->internal_id != INVALID_ID)
    {
        renderer_shader_release_instance_resources(shader_system_get_by_id(material->shader_id), material->internal_id);
        material->shader_id = INVALID_ID;
    }

    memory_zero(material, sizeof(Material));
    material->id = INVALID_ID;
    material->generation = INVALID_ID;
    material->internal_id = INVALID_ID;
}

bool create_default_material(MaterialSystemState* state)
{
    memory_zero(&state->default_material, sizeof(Material));
    state->default_material.id = INVALID_ID;
    state->default_material.generation = INVALID_ID;
    string_copy_n(state->default_material.name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    state->default_material.diffuse_color = vec4_one();
    state->default_material.diffuse_map.usage = TEXTURE_USE_DIFFUSE;
    state->default_material.diffuse_map.texture = texture_system_get_default();

    Shader* shader = shader_system_get(BUILTIN_SHADER_NAME_MATERIAL);
    if (!renderer_shader_acquire_instance_resources(shader, &state->default_material.internal_id))
    {
        log_error("Failed to create default material.");
        return false;
    }

    return true;
}

#define MATERIAL_APPLY_OR_FAIL(expr) if (!(expr)) { log_error("Failed to apply material: %s", expr); return false; }

bool material_system_apply_global(u64 shader_id, const Mat4* projection, const Mat4* view)
{
    if (shader_id == material_system_state->material_shader_id)
    {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->material_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->material_locations.view, view));
    }
    else if (shader_id == material_system_state->ui_shader_id)
    {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->ui_locations.projection, projection));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->ui_locations.view, view));
    }
    else 
    {
        log_error("Shader with id: %d is not supported by material system.", shader_id);
        return false;
    }

    MATERIAL_APPLY_OR_FAIL(shader_system_apply_global());
    return true;
}

bool material_system_apply_instance(Material* material)
{
    MATERIAL_APPLY_OR_FAIL(shader_system_bind_instance(material->internal_id));
    if (material->shader_id == material_system_state->material_shader_id)
    {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->material_locations.diffuse_color, &material->diffuse_color));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->material_locations.diffuse_texture, material->diffuse_map.texture));
    }
    else if (material->shader_id == material_system_state->ui_shader_id)
    {
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->ui_locations.diffuse_color, &material->diffuse_color));
        MATERIAL_APPLY_OR_FAIL(shader_system_uniform_set_by_id(material_system_state->ui_locations.diffuse_texture, material->diffuse_map.texture));
    }
    else 
    {
        log_error("Material shader with id: %d is not supported by material system.", material->shader_id);
        return false;
    }

    MATERIAL_APPLY_OR_FAIL(shader_system_apply_instance());
    return true;
}

bool material_system_apply_local(Material* material, const Mat4* model)
{
    if (material->shader_id == material_system_state->material_shader_id)
    {
        return shader_system_uniform_set_by_id(material_system_state->material_locations.model, model);
    }
    else if (material->shader_id == material_system_state->ui_shader_id)
    {
        return shader_system_uniform_set_by_id(material_system_state->ui_locations.model, model);
    }
    
    log_error("Material shader with id: %d is not supported by material system.", material->shader_id);
    return false;
}