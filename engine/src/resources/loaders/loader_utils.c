#include "resources/loaders/loader_utils.h"
#include <stddef.h>
#include "core/memory.h"
#include "lib/string.h"
#include "resources/resource_defines.h"
#include "core/log.h"

bool resource_unload(struct ResourceLoader* self, Resource* resource, MemoryTag tag)
{
    if (self == NULL || resource == NULL)
    {
        return false;
    }

    u32 path_length = string_length(resource->full_path);
    if (path_length > 0)
    {
        memory_free(resource->full_path, sizeof(char) * path_length - 1, MEMORY_TAG_STRING);
    }

    if (resource->data != NULL)
    {
        memory_free(resource->data, resource->size, tag);
        resource->data = NULL;
        resource->size = 0;
        resource->loader_id = INVALID_ID;
    }

    return true;
}