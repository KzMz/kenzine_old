#include "material_loader.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "resources/resource_defines.h"
#include "systems/resource_system.h"
#include "lib/math/math_defines.h"
#include "lib/math/vec4.h"
#include "platform/filesystem.h"
#include "vendor/json/json.h"

#include <stddef.h>

bool material_loader_load(ResourceLoader* self, const char* name, Resource* out_resource)
{
    if (self == NULL || name == NULL || out_resource == NULL)
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char path[512];
    string_format(path, format_str, resource_system_get_asset_base_path(), self->type_path, name, ".mat");

    out_resource->full_path = string_clone(path);

    FileHandle file_handle;
    if (!file_open(path, FILE_MODE_READ, false, &file_handle))
    {
        log_error("Failed to open material file: %s", path);
        return false;
    }

    MaterialResourceData* resource_data = (MaterialResourceData*) memory_alloc(sizeof(MaterialResourceData), MEMORY_TAG_MATERIALINSTANCE);
    resource_data->auto_release = true;
    resource_data->diffuse_color = vec4_one(); 
    resource_data->diffuse_map_name[0] = 0;
    string_copy_n(resource_data->name, name, MATERIAL_NAME_MAX_LENGTH);

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

    JsonNode* name_node = json_find_member(root, "name");
    if (name_node != NULL && name_node->tag == JSON_STRING)
    {
        string_copy_n(resource_data->name, name_node->string_, MATERIAL_NAME_MAX_LENGTH);
    }

    JsonNode* diffuse_map_node = json_find_member(root, "diffuse_map_name");
    if (diffuse_map_node != NULL && diffuse_map_node->tag == JSON_STRING)
    {
        string_copy_n(resource_data->diffuse_map_name, diffuse_map_node->string_, TEXTURE_NAME_MAX_LENGTH);
    }
    
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
    if (diffuse_color_node != NULL && diffuse_color_node->tag == JSON_OBJECT)
    {
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

        resource_data->diffuse_color = diffuse_color;
    }

    json_delete(root);
    file_close(&file_handle);

    out_resource->type = RESOURCE_TYPE_MATERIAL;
    out_resource->data = resource_data;
    out_resource->size = sizeof(MaterialResourceData);
    out_resource->name = name;

    return true;
}

bool material_loader_unload(ResourceLoader* self, Resource* resource)
{
    if (self == NULL || resource == NULL)
    {
        return false;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length > 0)
    {
        memory_free(resource->full_path, path_length, MEMORY_TAG_STRING);
    }

    if (resource->data == NULL)
    {
        return false;
    }

    memory_free(resource->data, resource->size, MEMORY_TAG_MATERIALINSTANCE);
    resource->data = NULL;
    resource->size = 0;
    resource->loader_id = INVALID_ID;

    return true;
}

ResourceLoader material_resource_loader_create(void)
{
    ResourceLoader loader = {0};
    loader.type = RESOURCE_TYPE_MATERIAL;
    loader.type_path = "materials";
    loader.load = material_loader_load;
    loader.unload = material_loader_unload;
    loader.custom_type = NULL;

    return loader;
}