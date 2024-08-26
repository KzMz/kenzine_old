#include "shader_loader.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "resources/resource_defines.h"
#include "systems/resource_system.h"
#include "lib/math/math_defines.h"
#include "resources/loaders/loader_utils.h"
#include "lib/containers/dyn_array.h"
#include "vendor/json/json.h"
#include "resources/json_utils.h"

#include "platform/filesystem.h"
#include <stddef.h>

bool shader_loader_load(ResourceLoader* self, const char* name, Resource* out_resource)
{
    if (self == NULL || name == NULL || out_resource == NULL)
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char path[512];
    string_format(path, format_str, resource_system_get_asset_base_path(), self->type_path, name, ".shader");

    FileHandle file_handle;
    if (!file_open(path, FILE_MODE_READ, false, &file_handle))
    {
        log_error("Failed to open shader file: %s", path);
        return false;
    }

    out_resource->full_path = string_clone(path);

    ShaderConfig* config = (ShaderConfig*) memory_alloc(sizeof(ShaderConfig), MEMORY_TAG_RESOURCE);
    config->attribute_count = 0;
    config->attributes = dynarray_create(ShaderAttributeConfig);
    config->uniform_count = 0;
    config->uniforms = dynarray_create(ShaderUniformConfig);
    config->stage_count = 0;
    config->stages = dynarray_create(ShaderStage);
    config->use_instances = false;
    config->use_local = false;
    config->stage_names = dynarray_create(char*);
    config->stage_files = dynarray_create(char*);
    config->renderpass_name = NULL;
    config->name = string_clone(name);

    char buffer[4096 * 2] = {0};
    u64 actual_size = 0;
    file_get_contents(&file_handle, (char*) &buffer, &actual_size);
    file_close(&file_handle);

    JsonNode* root = json_decode(buffer);
    if (root == NULL)
    {
        log_error("Failed to parse material config: %s", path);
        return false;
    }

    ResourceMetadata metadata;
    if (!json_utils_get_resource_metadata(RESOURCE_TYPE_SHADER, root, SHADER_NAME_MAX_LENGTH, &metadata))
    {
        log_error("Failed to get material metadata: %s", path);
        return false;
    }

    string_copy_n(config->name, metadata.name, SHADER_NAME_MAX_LENGTH);

    JsonNode* renderpass_node = json_find_member(root, "renderpass");
    if (renderpass_node == NULL)
    {
        log_error("Shader config missing renderpass field");
        return false;
    }
    if (renderpass_node->tag != JSON_STRING)
    {
        log_error("Shader config renderpass field is not a string");
        return false;
    }

    config->renderpass_name = string_clone(renderpass_node->string_);

    JsonNode* stages_node = json_find_member(root, "stages");
    if (stages_node == NULL)
    {
        log_error("Shader config missing stages field");
        return false;
    }
    if (stages_node->tag != JSON_ARRAY)
    {
        log_error("Shader config stages field is not an array");
        return false;
    }

    JsonNode* item = NULL;
    json_foreach(item, stages_node)
    {
        if (item->tag != JSON_OBJECT)
        {
            log_error("Shader config stage is not an object");
            return false;
        }

        JsonNode* stage_name_node = json_find_member(item, "stage");
        if (stage_name_node == NULL)
        {
            log_error("Shader config stage missing stage field");
            return false;
        }
        if (stage_name_node->tag != JSON_STRING)
        {
            log_error("Shader config stage stage field is not a string");
            return false;
        }

        dynarray_push(config->stage_names, string_clone(stage_name_node->string_));

        JsonNode* stage_file_node = json_find_member(item, "file");
        if (stage_file_node == NULL)
        {
            log_error("Shader config stage missing file field");
            return false;
        }

        if (stage_file_node->tag != JSON_STRING)
        {
            log_error("Shader config stage file field is not a string");
            return false;
        }

        dynarray_push(config->stage_files, string_clone(stage_file_node->string_));
    }
    config->stage_count = dynarray_length(config->stage_names);

    for (u8 i = 0; i < config->stage_count; ++i)
    {
        if (string_equals_nocase(config->stage_names[i], "frag") || string_equals_nocase(config->stage_names[i], "fragment"))
        {
            dynarray_push(config->stages, SHADER_STAGE_FRAGMENT);
        }
        else if (string_equals_nocase(config->stage_names[i], "vert") || string_equals_nocase(config->stage_names[i], "vertex"))
        {
            dynarray_push(config->stages, SHADER_STAGE_VERTEX);
        }
        else if (string_equals_nocase(config->stage_names[i], "geom") || string_equals_nocase(config->stage_names[i], "geometry"))
        {
            dynarray_push(config->stages, SHADER_STAGE_GEOMETRY);
        }
        else if (string_equals_nocase(config->stage_names[i], "comp") || string_equals_nocase(config->stage_names[i], "compute"))
        {
            dynarray_push(config->stages, SHADER_STAGE_COMPUTE);
        }
        else 
        {
            log_error("Unknown shader stage: %s", config->stage_names[i]);
            return false;
        }
    }

    JsonNode* use_instances_node = json_find_member(root, "use_instances");
    if (use_instances_node != NULL)
    {
        if (use_instances_node->tag != JSON_BOOL)
        {
            log_error("Shader config use_instance field is not a boolean");
            return false;
        }

        config->use_instances = use_instances_node->bool_;
    }

    JsonNode* use_local_node = json_find_member(root, "use_local");
    if (use_local_node != NULL)
    {
        if (use_local_node->tag != JSON_BOOL)
        {
            log_error("Shader config use_local field is not a boolean");
            return false;
        }

        config->use_local = use_local_node->bool_;
    }

    JsonNode* attributes_node = json_find_member(root, "attributes");
    if (attributes_node == NULL)
    {
        log_error("Shader config missing attributes field");
        return false;
    }
    if (attributes_node->tag != JSON_ARRAY)
    {
        log_error("Shader config attributes field is not an array");
        return false;
    }

    item = NULL;
    json_foreach(item, attributes_node)
    {
        if (item->tag != JSON_OBJECT)
        {
            log_error("Shader config attribute is not an object");
            return false;
        }

        ShaderAttributeConfig attribute = {0};
        
        JsonNode* name_node = json_find_member(item, "name");
        if (name_node == NULL)
        {
            log_error("Shader config attribute missing name field");
            return false;
        }
        if (name_node->tag != JSON_STRING)
        {
            log_error("Shader config attribute name field is not a string");
            return false;
        }

        attribute.name = string_clone(name_node->string_);
        attribute.name_length = string_length(attribute.name);

        JsonNode* type_node = json_find_member(item, "type");
        if (type_node == NULL)
        {
            log_error("Shader config attribute missing type field");
            return false;
        }
        if (type_node->tag != JSON_STRING)
        {
            log_error("Shader config attribute type field is not a string");
            return false;
        }

        if (string_equals_nocase(type_node->string_, "f32"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_FLOAT32;
            attribute.size = 4;
        }
        else if (string_equals_nocase(type_node->string_, "vec2"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_2;
            attribute.size = 8;
        }
        else if (string_equals_nocase(type_node->string_, "vec3"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_3;
            attribute.size = 12;
        }
        else if (string_equals_nocase(type_node->string_, "vec4"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_FLOAT32_4;
            attribute.size = 16;
        }
        else if (string_equals_nocase(type_node->string_, "u8"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_UINT8;
            attribute.size = 1;
        }
        else if (string_equals_nocase(type_node->string_, "u16"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_UINT16;
            attribute.size = 2;
        }
        else if (string_equals_nocase(type_node->string_, "u32"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_UINT32;
            attribute.size = 4;
        }
        else if (string_equals_nocase(type_node->string_, "i8"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_INT8;
            attribute.size = 1;
        }
        else if (string_equals_nocase(type_node->string_, "i16"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_INT16;
            attribute.size = 2;
        }
        else if (string_equals_nocase(type_node->string_, "i32"))
        {
            attribute.type = SHADER_ATTRIB_TYPE_INT32;
            attribute.size = 4;
        }
        else
        {
            log_error("Unknown shader attribute type: %s", type_node->string_);
            log_warning("Defaulting to f32");
            attribute.type = SHADER_ATTRIB_TYPE_FLOAT32;
            attribute.size = 4;
        }

        dynarray_push(config->attributes, attribute);
        config->attribute_count++;
    }

    JsonNode* uniforms_node = json_find_member(root, "uniforms");
    if (uniforms_node == NULL)
    {
        log_error("Shader config missing uniforms field");
        return false;
    }
    if (uniforms_node->tag != JSON_ARRAY)
    {
        log_error("Shader config uniforms field is not an array");
        return false;
    }

    item = NULL;
    json_foreach(item, uniforms_node)
    {
        if (item->tag != JSON_OBJECT)
        {
            log_error("Shader config uniform is not an object");
            return false;
        }

        ShaderUniformConfig uniform = {0};

        JsonNode* name_node = json_find_member(item, "name");
        if (name_node == NULL)
        {
            log_error("Shader config uniform missing name field");
            return false;
        }
        if (name_node->tag != JSON_STRING)
        {
            log_error("Shader config uniform name field is not a string");
            return false;
        }

        uniform.name = string_clone(name_node->string_);
        uniform.name_length = string_length(uniform.name);

        JsonNode* scope_node = json_find_member(item, "scope");
        if (scope_node == NULL)
        {
            log_error("Shader config uniform missing scope field");
            return false;
        }
        if (scope_node->tag != JSON_STRING)
        {
            log_error("Shader config uniform scope field is not a string");
            return false;
        }

        if (string_equals_nocase(scope_node->string_, "global"))
        {
            uniform.scope = SHADER_SCOPE_GLOBAL;
        }
        else if (string_equals_nocase(scope_node->string_, "instance"))
        {
            uniform.scope = SHADER_SCOPE_INSTANCE;
        }
        else if (string_equals_nocase(scope_node->string_, "local"))
        {
            uniform.scope = SHADER_SCOPE_LOCAL;
        }
        else
        {
            log_error("Unknown shader uniform scope: %s", scope_node->string_);
            log_warning("Defaulting to global");
            uniform.scope = SHADER_SCOPE_GLOBAL;
        }

        JsonNode* type_node = json_find_member(item, "type");
        if (type_node == NULL)
        {
            log_error("Shader config uniform missing type field");
            return false;
        }
        if (type_node->tag != JSON_STRING)
        {
            log_error("Shader config uniform type field is not a string");
            return false;
        }

        if (string_equals_nocase(type_node->string_, "f32"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_FLOAT32;
            uniform.size = 4;
        }
        else if (string_equals_nocase(type_node->string_, "vec2"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_2;
            uniform.size = 8;
        }
        else if (string_equals_nocase(type_node->string_, "vec3"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_3;
            uniform.size = 12;
        }
        else if (string_equals_nocase(type_node->string_, "vec4"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_FLOAT32_4;
            uniform.size = 16;
        }
        else if (string_equals_nocase(type_node->string_, "u8"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_UINT8;
            uniform.size = 1;
        }
        else if (string_equals_nocase(type_node->string_, "u16"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_UINT16;
            uniform.size = 2;
        }
        else if (string_equals_nocase(type_node->string_, "u32"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_UINT32;
            uniform.size = 4;
        }
        else if (string_equals_nocase(type_node->string_, "i8"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_INT8;
            uniform.size = 1;
        }
        else if (string_equals_nocase(type_node->string_, "i16"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_INT16;
            uniform.size = 2;
        }
        else if (string_equals_nocase(type_node->string_, "i32"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_INT32;
            uniform.size = 4;
        }
        else if (string_equals_nocase(type_node->string_, "mat4"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_MATRIX_4;
            uniform.size = 64;
        }
        else if (string_equals_nocase(type_node->string_, "samp") || string_equals_nocase(type_node->string_, "sampler"))
        {
            uniform.type = SHADER_UNIFORM_TYPE_SAMPLER;
            uniform.size = 0;
        }
        else
        {
            log_error("Unknown shader uniform type: %s", type_node->string_);
            log_warning("Defaulting to f32");
            uniform.type = SHADER_UNIFORM_TYPE_FLOAT32;
            uniform.size = 4;
        }

        dynarray_push(config->uniforms, uniform);
        config->uniform_count++;
    }

    json_delete(root);
    file_close(&file_handle);

    out_resource->type = RESOURCE_TYPE_SHADER;
    out_resource->data = config;
    out_resource->size = sizeof(ShaderConfig);
    out_resource->name = name;

    return true;
}

bool shader_loader_unload(ResourceLoader* self, Resource* resource)
{
    ShaderConfig* config = (ShaderConfig*) resource->data;

    dynarray_destroy(config->stage_files);
    dynarray_destroy(config->stage_names);
    dynarray_destroy(config->stages);

    u32 count = dynarray_length(config->attributes);
    for (u32 i = 0; i < count; ++i)
    {
        u32 length = string_length(config->attributes[i].name);
        memory_free(config->attributes[i].name, (length + 1) * sizeof(char), MEMORY_TAG_STRING);
    }
    dynarray_destroy(config->attributes);

    count = dynarray_length(config->uniforms);
    for (u32 i = 0; i < count; ++i)
    {
        u32 length = string_length(config->uniforms[i].name);
        memory_free(config->uniforms[i].name, (length + 1) * sizeof(char), MEMORY_TAG_STRING);
    }
    dynarray_destroy(config->uniforms);

    memory_free(config->renderpass_name, sizeof(char) * (string_length(config->renderpass_name) + 1), MEMORY_TAG_STRING);
    memory_free(config->name, sizeof(char) * (string_length(config->name) + 1), MEMORY_TAG_STRING);
    memory_zero(config, sizeof(ShaderConfig));

    if (!resource_unload(self, resource, MEMORY_TAG_RESOURCE))
    {
        log_error("Failed to unload shader resource.");
        return false;
    }

    return true;
}

ResourceLoader shader_resource_loader_create(void)
{
    ResourceLoader loader = { 0 };
    loader.type = RESOURCE_TYPE_SHADER;
    loader.custom_type = NULL;
    loader.load = shader_loader_load;
    loader.unload = shader_loader_unload;
    loader.type_path = "shaders";

    return loader;
}