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

// TODO: resource system
#include "platform/filesystem.h"
#include "vendor/json/json.h"

typedef struct MaterialSystemState
{
    MaterialSystemConfig config;

    Material default_material;

    Material* materials;
    HashTable material_table;
} MaterialSystemState;

typedef struct MaterialReference
{
    u64 reference_count;
    u32 handle;
    bool auto_release;
} MaterialReference;

static MaterialSystemState* material_system_state = NULL;

bool create_default_material(MaterialSystemState* state);
bool load_material(MaterialConfig config, Material* out_material);
void destroy_material(Material* material);
bool load_material_config(const char* path, MaterialConfig* out_config);

bool material_system_init(void* state, MaterialSystemConfig config)
{
    if (config.max_materials == 0)
    {
        log_error("Material system config is invalid. Max materials must be greater than 0.");
        return false;
    }

    material_system_state = (MaterialSystemState*) state;
    material_system_state->config = config;
    material_system_state->materials = dynarray_reserve(Material, config.max_materials);
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

    dynarray_destroy(material_system_state->materials);
    hashtable_destroy(&material_system_state->material_table);
    memory_zero(material_system_state, sizeof(MaterialSystemState));
    material_system_state = NULL;
}

u64 material_system_get_state_size(void)
{
    return sizeof(MaterialSystemState);
}

Material* material_system_acquire(const char* name)
{
    MaterialConfig config;
    
    char* format_str = "../assets/materials/%s.%s";
    char full_file_path[512];

    string_format(full_file_path, format_str, name, "mat");
    if (!load_material_config(full_file_path, &config))
    {
        log_error("Failed to load material config: %s", name);
        return NULL;
    }

    return material_system_acquire_from_config(config);
}

Material* material_system_acquire_from_config(MaterialConfig config)
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

bool load_material(MaterialConfig config, Material* out_material)
{
    memory_zero(out_material, sizeof(Material));

    string_copy_n(out_material->name, config.name, MATERIAL_NAME_MAX_LENGTH);
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

    if (!renderer_create_material(out_material))
    {
        log_error("Failed to create material: %s", config.name);
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

    renderer_destroy_material(material);

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

    if (!renderer_create_material(&state->default_material))
    {
        log_error("Failed to create default material.");
        return false;
    }

    return true;
}

bool load_material_config(const char* path, MaterialConfig* out_config)
{
    FileHandle file_handle;
    if (!file_open(path, FILE_MODE_READ, false, &file_handle))
    {
        log_error("Failed to open material config file: %s", path);
        return false;
    }

    char buffer[4096] = {0};
    u64 actual_size = 0;
    file_get_contents(&file_handle, (char*) &buffer, &actual_size);
    file_close(&file_handle);

    JsonNode* root = json_decode(buffer);
    if (root == NULL)
    {
        log_error("Failed to parse material config: %s", path);
        return false;
    }

    char type[64];
    JsonNode* type_node = json_find_member(root, "type");
    if (type_node == NULL)
    {
        log_error("Material config missing type field: %s", path);
        return false;
    }
    if (type_node->tag != JSON_STRING)
    {
        log_error("Material config type field is not a string: %s", path);
        return false;
    }
    if (!string_equals_nocase(type_node->string_, "material"))
    {
        log_error("Material config type field is not a material: %s", path);
        return false;
    }

    char name[MATERIAL_NAME_MAX_LENGTH];
    JsonNode* name_node = json_find_member(root, "name");
    if (name_node == NULL)
    {
        log_error("Material config missing name field: %s", path);
        return false;
    }
    if (name_node->tag != JSON_STRING)
    {
        log_error("Material config name field is not a string: %s", path);
        return false;
    }

    string_copy_n(out_config->name, name_node->string_, MATERIAL_NAME_MAX_LENGTH);

    char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
    JsonNode* diffuse_map_node = json_find_member(root, "diffuse_map_name");
    if (diffuse_map_node == NULL)
    {
        log_error("Material config missing diffuse_map field: %s", path);
        return false;
    }
    if (diffuse_map_node->tag != JSON_STRING)
    {
        log_error("Material config diffuse_map field is not a string: %s", path);
        return false;
    }

    string_copy_n(out_config->diffuse_map_name, diffuse_map_node->string_, TEXTURE_NAME_MAX_LENGTH);

    f32 version = 0.0f;
    JsonNode* version_node = json_find_member(root, "version");
    if (version_node == NULL)
    {
        log_error("Material config missing version field: %s", path);
        return false;
    }
    if (version_node->tag != JSON_NUMBER)
    {
        log_error("Material config version field is not a number: %s", path);
        return false;
    }

    Vec4 diffuse_color = vec4_zero();
    JsonNode* diffuse_color_node = json_find_member(root, "diffuse_color");
    if (diffuse_color_node == NULL)
    {
        log_error("Material config missing diffuse_color field: %s", path);
        return false;
    }
    if (diffuse_color_node->tag != JSON_OBJECT)
    {
        log_error("Material config diffuse_color field is not an array: %s", path);
        return false;
    }

    f32 diffuse_color_r = 0.0f;
    JsonNode* diffuse_color_r_node = json_find_member(diffuse_color_node, "r");
    if (diffuse_color_r_node == NULL)
    {
        log_error("Material config missing diffuse_color r field: %s", path);
        return false;
    }
    if (diffuse_color_r_node->tag != JSON_NUMBER)
    {
        log_error("Material config diffuse_color r field is not a number: %s", path);
        return false;
    }

    f32 diffuse_color_g = 0.0f;
    JsonNode* diffuse_color_g_node = json_find_member(diffuse_color_node, "g");
    if (diffuse_color_g_node == NULL)
    {
        log_error("Material config missing diffuse_color g field: %s", path);
        return false;
    }
    if (diffuse_color_g_node->tag != JSON_NUMBER)
    {
        log_error("Material config diffuse_color g field is not a number: %s", path);
        return false;
    }

    f32 diffuse_color_b = 0.0f;
    JsonNode* diffuse_color_b_node = json_find_member(diffuse_color_node, "b");
    if (diffuse_color_b_node == NULL)
    {
        log_error("Material config missing diffuse_color b field: %s", path);
        return false;
    }
    if (diffuse_color_b_node->tag != JSON_NUMBER)
    {
        log_error("Material config diffuse_color b field is not a number: %s", path);
        return false;
    }

    f32 diffuse_color_a = 0.0f;
    JsonNode* diffuse_color_a_node = json_find_member(diffuse_color_node, "a");
    if (diffuse_color_a_node == NULL)
    {
        log_error("Material config missing diffuse_color a field: %s", path);
        return false;
    }
    if (diffuse_color_a_node->tag != JSON_NUMBER)
    {
        log_error("Material config diffuse_color a field is not a number: %s", path);
        return false;
    }

    diffuse_color.r = (f32) diffuse_color_r_node->number_;
    diffuse_color.g = (f32) diffuse_color_g_node->number_;
    diffuse_color.b = (f32) diffuse_color_b_node->number_;
    diffuse_color.a = (f32) diffuse_color_a_node->number_;

    out_config->diffuse_color = diffuse_color;
    return true;
}