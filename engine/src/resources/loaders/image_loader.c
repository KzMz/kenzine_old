#include "resources/loaders/image_loader.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "resources/resource_defines.h"
#include "systems/resource_system.h"
#include "resources/loaders/loader_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

bool image_loader_load(ResourceLoader* self, const char* name, Resource* out_resource)
{
    if (self == NULL || name == NULL || out_resource == NULL)
    {
        return false;
    }

    char* format_str = "%s/%s/%s%s";
    const i32 required_channel_count = 4;

    stbi_set_flip_vertically_on_load(true);

    const i32 max_path_length = 512;
    char path[max_path_length];
    string_format(path, format_str, resource_system_get_asset_base_path(), self->type_path, name, ".png");

    i32 width, height, channel_count;
    u8* data = stbi_load(path, &width, &height, &channel_count, required_channel_count);

    const char* fail_reason = stbi_failure_reason();
    if (fail_reason != NULL)
    {
        log_error("Failed to load image: %s. Reason: %s", path, fail_reason);
        stbi__err(0, 0);

        if (data != NULL)
        {
            stbi_image_free(data);
        }

        return false;
    }

    if (data == NULL)
    {
        log_error("Failed to load image: %s. Reason: Unknown", path);
        return false;
    }

    out_resource->full_path = string_clone(path);

    ImageResourceData* image_data = (ImageResourceData*) memory_alloc(sizeof(ImageResourceData), MEMORY_TAG_TEXTURE);
    image_data->channel_count = required_channel_count;
    image_data->width = (u32) width;
    image_data->height = (u32) height;
    image_data->pixels = data;

    out_resource->type = RESOURCE_TYPE_IMAGE;
    out_resource->data = image_data;
    out_resource->size = sizeof(ImageResourceData);
    out_resource->name = name;

    return true;
}

bool image_loader_unload(ResourceLoader* self, Resource* resource)
{
    return resource_unload(self, resource, MEMORY_TAG_TEXTURE);
}

ResourceLoader image_resource_loader_create(void)
{
    ResourceLoader loader = {0};
    loader.type = RESOURCE_TYPE_IMAGE;
    loader.custom_type = NULL;
    loader.load = image_loader_load;
    loader.unload = image_loader_unload;
    loader.type_path = "textures";

    return loader;
}