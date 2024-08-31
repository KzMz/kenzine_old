#include "device_loader.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "resources/resource_defines.h"
#include "systems/resource_system.h"
#include "platform/filesystem.h"
#include "vendor/json/json.h"
#include "resources/json_utils.h"
#include "loader_utils.h"
#include "lib/containers/dyn_array.h"

#include <stddef.h>

bool device_loader_load(ResourceLoader* self, const char* name, Resource* out_resource)
{
    if (self == NULL || name == NULL || out_resource == NULL)
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char path[512];
    string_format(path, format_str, resource_system_get_asset_base_path(), self->type_path, name, ".device");

    FileHandle file_handle;
    if (!file_open(path, FILE_MODE_READ, false, &file_handle))
    {
        log_error("Failed to open device file: %s", path);
        return false;
    }

    out_resource->full_path = string_clone(path);

    DeviceConfig* config = (DeviceConfig*) memory_alloc(sizeof(DeviceConfig), MEMORY_TAG_RESOURCE);
    config->name = string_clone(name);
    config->actions_count = 0;
    config->actions = dynarray_create(DeviceInputActionConfig);
    
    hashtable_create(u32, 128, false, &config->keys);

    config->type = DEVICE_TYPE_UNKNOWN;
    config->gamepad_type = DEVICE_TYPE_GAMEPAD_NONE;

    char buffer[4096 * 2] = {0};
    u64 actual_size = 0;
    file_get_contents(&file_handle, (char*) &buffer, &actual_size);
    file_close(&file_handle);

    JsonNode* root = json_decode(buffer);
    if (root == NULL)
    {
        log_error("Failed to parse device config: %s", path);
        return false;
    }

    ResourceMetadata metadata;
    if (!json_utils_get_resource_metadata(RESOURCE_TYPE_DEVICE, root, DEVICE_NAME_MAX_LENGTH, &metadata))
    {
        log_error("Failed to get device metadata: %s", path);
        return false;
    }

    string_copy_n(config->name, metadata.name, DEVICE_NAME_MAX_LENGTH);

    config->sub_id = DEVICE_SUB_ID_ANY;
    JsonNode* sub_id_node = json_find_member(root, "sub_id");
    if (sub_id_node != NULL)
    {
        if (sub_id_node->tag != JSON_NUMBER)
        {
            log_error("Device config sub_id field is not a number");
            return false;
        }

        config->sub_id = (i32) sub_id_node->number_;
    }

    JsonNode* type_node = json_find_member(root, "type");
    if (type_node == NULL)
    {
        log_error("Device config missing type field");
        return false;
    }
    if (type_node->tag != JSON_STRING)
    {
        log_error("Device config type field is not a string");
        return false;
    }

    if (string_equals_nocase(type_node->string_, "keyboard"))
    {
        config->type = DEVICE_TYPE_KEYBOARD;
    }
    else if (string_equals_nocase(type_node->string_, "mouse"))
    {
        config->type = DEVICE_TYPE_MOUSE;
    }
    else if (string_equals_nocase(type_node->string_, "gamepad"))
    {
        config->type = DEVICE_TYPE_GAMEPAD;
    }
    else
    {
        log_error("Unknown device type: %s", type_node->string_);
        return false;
    }

    if (config->type == DEVICE_TYPE_GAMEPAD)
    {
        JsonNode* gamepad_type_node = json_find_member(root, "gamepad");
        if (gamepad_type_node == NULL)
        {
            log_error("Gamepad config missing gamepad_type field");
            return false;
        }
        if (gamepad_type_node->tag != JSON_STRING)
        {
            log_error("Gamepad config gamepad_type field is not a string");
            return false;
        }

        if (string_equals_nocase(gamepad_type_node->string_, "xbox"))
        {
            config->gamepad_type = DEVICE_TYPE_GAMEPAD_XBOX;
        }
        else if (string_equals_nocase(gamepad_type_node->string_, "dualshock4"))
        {
            config->gamepad_type = DEVICE_TYPE_GAMEPAD_DUALSHOCK4;
        }
        else
        {
            config->gamepad_type = DEVICE_TYPE_GAMEPAD_GENERIC;
        }
    }

    JsonNode* keys_node = json_find_member(root, "keys");
    if (keys_node == NULL)
    {
        log_error("Device config missing keys field");
        return false;
    }
    if (keys_node->tag != JSON_ARRAY)
    {
        log_error("Device config keys field is not an array");
        return false;
    }

    JsonNode* item = NULL;
    json_foreach(item, keys_node)
    {
        if (item->tag != JSON_OBJECT)
        {
            log_error("Device config key is not an object");
            return false;
        }

        JsonNode* key_name_node = json_find_member(item, "name");
        if (key_name_node == NULL)
        {
            log_error("Device config key missing name field");
            return false;
        }
        if (key_name_node->tag != JSON_STRING)
        {
            log_error("Device config key name field is not a string");
            return false;
        }

        JsonNode* key_code_node = json_find_member(item, "code");
        if (key_code_node == NULL)
        {
            log_error("Device config key missing key_code field");
            return false;
        }
        if (key_code_node->tag != JSON_NUMBER)
        {
            log_error("Device config key key_code field is not a number");
            return false;
        }

        u32 key_code = (u32) key_code_node->number_;
        hashtable_set(&config->keys, key_name_node->string_, &key_code);
    }

    JsonNode* actions_node = json_find_member(root, "actions");
    if (actions_node == NULL)
    {
        log_error("Device config missing actions field");
        return false;
    }
    if (actions_node->tag != JSON_ARRAY)
    {
        log_error("Device config actions field is not an array");
        return false;
    }

    item = NULL;
    json_foreach(item, actions_node)
    {
        if (item->tag != JSON_OBJECT)
        {
            log_error("Device config action is not an object");
            return false;
        }

        DeviceInputActionConfig action = {0};

        JsonNode* action_name_node = json_find_member(item, "name");
        if (action_name_node == NULL)
        {
            log_error("Device config action missing name field");
            return false;
        }
        if (action_name_node->tag != JSON_STRING)
        {
            log_error("Device config action name field is not a string");
            return false;
        }

        string_copy_n(action.action_name, action_name_node->string_, MAX_INPUTACTION_NAME_LENGTH);

        JsonNode* action_type_node = json_find_member(item, "type");
        if (action_type_node == NULL)
        {
            log_error("Device config action missing type field");
            return false;
        }
        if (action_type_node->tag != JSON_STRING)
        {
            log_error("Device config action type field is not a string");
            return false;
        }

        if (string_equals_nocase(action_type_node->string_, "button"))
        {
            action.action_type = INPUT_ACTION_TYPE_BUTTON;
        }
        else if (string_equals_nocase(action_type_node->string_, "axis"))
        {
            action.action_type = INPUT_ACTION_TYPE_AXIS;
        }
        else
        {
            log_error("Unknown device action type: %s", action_type_node->string_);
            return false;
        }

        if (action.action_type == INPUT_ACTION_TYPE_AXIS)
        {
            action.inverted = false;
            JsonNode* inverted_node = json_find_member(item, "inverted");
            if (inverted_node != NULL)
            {
                if (inverted_node->tag != JSON_BOOL)
                {
                    log_error("Device config action inverted field is not a boolean");
                    return false;
                }
                action.inverted = inverted_node->bool_;
            }

            action.deadzone = 0.0f;
            JsonNode* deadzone_node = json_find_member(item, "deadzone");
            if (deadzone_node != NULL)
            {
                if (deadzone_node->tag != JSON_NUMBER)
                {
                    log_error("Device config action deadzone field is not a number");
                    return false;
                }
                action.deadzone = (f32) deadzone_node->number_;
            }

            JsonNode* action_native = json_find_member(item, "native");
            if (action_native != NULL)
            {
                if (action_native->tag != JSON_STRING)
                {
                    log_error("Device config action native field is not a string");
                    return false;
                }
                string_copy_n(action.native_axis_key_name, action_native->string_, DEVICE_KEY_NAME_MAX_LENGTH);
                action.negative_axis_key_name[0] = 0;
                action.positive_axis_key_name[0] = 0;
                action.axis_type = INPUT_ACTION_AXIS_TYPE_NATIVE;
            }
            else 
            {
                JsonNode* action_positive = json_find_member(item, "positive");
                if (action_positive == NULL)
                {
                    log_error("Device config action missing positive field");
                    return false;
                }
                if (action_positive->tag != JSON_STRING)
                {
                    log_error("Device config action positive field is not a string");
                    return false;
                }
                string_copy_n(action.positive_axis_key_name, action_positive->string_, DEVICE_KEY_NAME_MAX_LENGTH);

                JsonNode* action_negative = json_find_member(item, "negative");
                if (action_negative == NULL)
                {
                    log_error("Device config action missing negative field");
                    return false;
                }
                if (action_negative->tag != JSON_STRING)
                {
                    log_error("Device config action negative field is not a string");
                    return false;
                }
                string_copy_n(action.negative_axis_key_name, action_negative->string_, DEVICE_KEY_NAME_MAX_LENGTH);
                action.native_axis_key_name[0] = 0;
                action.axis_type = INPUT_ACTION_AXIS_TYPE_VIRTUAL;
            }
        }
        else if (action.action_type == INPUT_ACTION_TYPE_BUTTON)
        {
            JsonNode* action_key_node = json_find_member(item, "key");
            if (action_key_node == NULL)
            {
                log_error("Device config action missing key field");
                return false;
            }
            if (action_key_node->tag != JSON_STRING)
            {
                log_error("Device config action key field is not a string");
                return false;
            }

            string_copy_n(action.key_name, action_key_node->string_, DEVICE_KEY_NAME_MAX_LENGTH);
            action.native_axis_key_name[0] = 0;
            action.positive_axis_key_name[0] = 0;
            action.negative_axis_key_name[0] = 0;
        }

        dynarray_push(config->actions, action);
    }
    config->actions_count = dynarray_length(config->actions);

    json_delete(root);

    out_resource->data = config;
    out_resource->size = sizeof(DeviceConfig);
    out_resource->type = RESOURCE_TYPE_DEVICE;

    return true;
}

bool device_loader_unload(ResourceLoader* self, Resource* resource)
{
    return resource_unload(self, resource, MEMORY_TAG_INPUTDEVICE);
}

ResourceLoader device_resource_loader_create(void)
{
    ResourceLoader loader = {0};
    loader.type = RESOURCE_TYPE_DEVICE;
    loader.type_path = "devices";
    loader.load = device_loader_load;
    loader.unload = device_loader_unload;
    return loader;
}