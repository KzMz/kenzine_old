#include "resources/loaders/binary_loader.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "resources/resource_defines.h"
#include "systems/resource_system.h"
#include "platform/filesystem.h"

#include <stddef.h>

bool binary_loader_load(ResourceLoader* self, const char* name, Resource* out_resource)
{
    if (self == NULL || name == NULL || out_resource == NULL)
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    char path[512];
    string_format(path, format_str, resource_system_get_asset_base_path(), self->type_path, name, "");

    out_resource->full_path = string_clone(path);

    FileHandle file_handle;
    if (!file_open(path, FILE_MODE_READ, true, &file_handle))
    {
        log_error("Failed to open binary file: %s", path);
        return false;
    }

    u64 size = 0;
    if (!file_size(&file_handle, &size))
    {
        log_error("Failed to get size of binary file: %s", path);
        file_close(&file_handle);
        return false;
    }

    u8* resource_data = (u8*) memory_alloc(size, MEMORY_TAG_BINARY);
    u64 read_size = 0;
    if (!file_read_all_bytes(&file_handle, resource_data, &read_size))
    {
        log_error("Failed to read binary file: %s", path);
        memory_free(resource_data, size, MEMORY_TAG_BINARY);
        file_close(&file_handle);
        return false;
    }

    file_close(&file_handle);

    out_resource->type = RESOURCE_TYPE_BINARY;
    out_resource->data = resource_data;
    out_resource->size = size;
    out_resource->name = name;

    return true;
}

bool binary_loader_unload(ResourceLoader* self, Resource* resource)
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

    if (resource->data != NULL)
    {
        memory_free(resource->data, resource->size, MEMORY_TAG_BINARY);
        resource->data = NULL;
        resource->size = 0;
        resource->loader_id = INVALID_ID;
    }

    return true;
}

ResourceLoader binary_resource_loader_create(void)
{
    ResourceLoader loader = {0};
    loader.type = RESOURCE_TYPE_BINARY;
    loader.type_path = "";
    loader.load = binary_loader_load;
    loader.unload = binary_loader_unload;
    loader.custom_type = NULL;

    return loader;
}