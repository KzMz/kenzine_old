#include "resources/json_utils.h"

#include "core/log.h"
#include "lib/string.h"
#include "vendor/json/json.h"
#include <stddef.h>

const char* resource_type_to_string(ResourceType type)
{
    switch (type)
    {
        case RESOURCE_TYPE_MATERIAL:
            return "material";
        case RESOURCE_TYPE_TEXT:
            return "text";
        case RESOURCE_TYPE_BINARY:
            return "binary";
        case RESOURCE_TYPE_IMAGE:
            return "image";
        case RESOURCE_TYPE_STATIC_MESH:
            return "static_mesh";
        case RESOURCE_TYPE_SHADER:
            return "shader";
        case RESOURCE_TYPE_CUSTOM:
            return "custom";
        default:
            return "unknown";
    }
}

bool json_utils_get_resource_metadata(ResourceType type, struct JsonNode* root, u32 resource_name_size, ResourceMetadata* out_metadata)
{
    if (root == NULL)
    {
        log_error("json node is NULL");
        return false;
    }
    if (out_metadata == NULL)
    {
        log_error("out_metadata is NULL");
        return false;
    }

    char resource_name[64];
    JsonNode* resource_node = json_find_member(root, "resource");
    if (resource_node == NULL)
    {
        log_error("Resource config missing resource field");
        return false;
    }
    if (resource_node->tag != JSON_OBJECT)
    {
        log_error("Resource config resource field is not an object");
        return false;
    }

    JsonNode* type_node = json_find_member(resource_node, "type");
    if (type_node == NULL)
    {
        log_error("Resource config missing type field");
        return false;
    }

    const char* expected_type = resource_type_to_string(type);
    if (string_equals_nocase(type_node->string_, expected_type) == false)
    {
        log_error("Resource config resource field is not a %s but %s", expected_type, type_node->string_);
        return false;
    }

    out_metadata->type = type;

    JsonNode* name_node = json_find_member(resource_node, "name");
    if (name_node == NULL)
    {
        log_error("Resource config missing name field");
        return false;
    }
    if (name_node->tag != JSON_STRING)
    {
        log_error("Resource config name field is not a string");
        return false;
    }

    string_copy_n(out_metadata->name, name_node->string_, resource_name_size);

    JsonNode* version_node = json_find_member(resource_node, "version");
    if (version_node == NULL)
    {
        log_error("Resource config missing version field");
        return false;
    }
    if (version_node->tag != JSON_STRING)
    {
        log_error("Resource config version field is not a number");
        return false;
    }

    string_copy_n(out_metadata->version, version_node->string_, RESOURCE_VERSION_MAX_LENGTH);

    JsonNode* custom_type_node = json_find_member(resource_node, "custom_type");
    if (custom_type_node != NULL)
    {
        if (custom_type_node->tag != JSON_STRING)
        {
            log_error("Resource config custom_type field is not a string");
            return false;
        }

        string_copy_n(out_metadata->custom_type, custom_type_node->string_, RESOURCE_CUSTOM_TYPE_MAX_LENGTH);
    }

    return true;
}

const char* json_utils_get_string(struct JsonNode* parent, const char* key)
{
    if (parent == NULL)
    {
        log_error("json node is NULL");
        return NULL;
    }
    JsonNode* node = json_find_member(parent, key);
    if (node == NULL)
    {
        log_error("Resource config missing %s field", key);
        return NULL;
    }
    if (node->tag != JSON_STRING)
    {
        log_error("Resource config %s field is not a string", key);
        return NULL;
    }
    return node->string_;
}