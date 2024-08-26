#include "resource_system.h"

#include <stddef.h>
#include "core/log.h"
#include "lib/string.h"

#include "resources/loaders/text_loader.h"
#include "resources/loaders/image_loader.h"
#include "resources/loaders/material_loader.h"
#include "resources/loaders/binary_loader.h"
#include "resources/loaders/shader_loader.h"

typedef struct ResourceSystemState
{
    ResourceSystemConfig config;
    ResourceLoader* loaders;
} ResourceSystemState;

static ResourceSystemState* resource_system_state = NULL;

bool load_resource(const char* name, ResourceLoader* loader, Resource* out_resource);

bool resource_system_init(void* state, ResourceSystemConfig config)
{
    if (resource_system_state != NULL)
    {
        log_error("Resource system already initialized.");
        return false;
    }

    if (config.asset_base_path == NULL)
    {
        log_error("Asset base path not set.");
        return false;
    }

    resource_system_state = (ResourceSystemState*) state;
    resource_system_state->config = config;
    resource_system_state->loaders = state + sizeof(ResourceSystemState);

    for (u32 i = 0; i < config.max_loaders; i++)
    {
        resource_system_state->loaders[i].id = INVALID_ID;
    }

    resource_system_register_loader(text_resource_loader_create());
    resource_system_register_loader(binary_resource_loader_create());
    resource_system_register_loader(image_resource_loader_create());
    resource_system_register_loader(material_resource_loader_create());
    resource_system_register_loader(shader_resource_loader_create());

    log_info("Resource system initialized. [Base path: %s]", config.asset_base_path);
    return true;
}

void resource_system_shutdown(void)
{
    if (resource_system_state == NULL)
    {
        log_error("Resource system not initialized.");
        return;
    }

    resource_system_state = NULL;
    log_info("Resource system shut down.");
}

u64 resource_system_get_state_size(ResourceSystemConfig config)
{
    return sizeof(ResourceSystemState) + (sizeof(ResourceLoader) * config.max_loaders);
}

bool resource_system_register_loader(ResourceLoader loader)
{
    if (resource_system_state == NULL)
    {
        log_error("Resource system not initialized.");
        return false;
    }

    u32 count = resource_system_state->config.max_loaders;
    // Search for another loader with the same type or custom type
    for (u32 i = 0; i < count; ++i)
    {
        ResourceLoader* current_loader = &resource_system_state->loaders[i];
        if (current_loader->id == INVALID_ID) continue;

        if (current_loader->type == loader.type && loader.type != RESOURCE_TYPE_CUSTOM)
        {
            log_error("Resource loader already registered. [Type: %d]", loader.type);
            return false;
        }
        if (loader.type == RESOURCE_TYPE_CUSTOM && loader.custom_type != NULL && string_length(loader.custom_type) > 0)
        {
            // NOTE: see if case-insensitive is a good idea or not
            if (string_equals_nocase(current_loader->custom_type, loader.custom_type))
            {
                log_error("Resource loader already registered. [Custom type: %s]", loader.custom_type);
                return false;
            }
        }
    }

    for (u32 i = 0; i < count; ++i)
    {
        if (resource_system_state->loaders[i].id == INVALID_ID)
        {
            resource_system_state->loaders[i] = loader;
            resource_system_state->loaders[i].id = i;
            log_trace("Resource loader registered. [Type: %d]", loader.type);
            return true;
        }
    }

    log_error("Resource loader limit reached.");
    return false;
}

bool resource_system_load(const char* name, ResourceType type, Resource* out_resource)
{
    if (resource_system_state == NULL)
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Resource system not initialized.");
        return false;
    }
    if (type == RESOURCE_TYPE_CUSTOM)
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Resource type not supported by <resource_system_load>. Please call <resource_system_load_custom> instead.");
        return false;
    }

    for (u32 i = 0; i < resource_system_state->config.max_loaders; i++)
    {
        ResourceLoader* loader = &resource_system_state->loaders[i];
        if (loader->id == INVALID_ID) continue;

        if (loader->type == type)
        {
            return load_resource(name, loader, out_resource);
        }
    }

    out_resource->loader_id = INVALID_ID;
    log_error("Resource loader not found. [Type: %d]", type);
    return false;
}

bool resource_system_load_custom(const char* name, const char* custom_type, Resource* out_resource)
{
    if (resource_system_state == NULL)
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Resource system not initialized.");
        return false;
    }

    if (custom_type == NULL || string_length(custom_type) == 0)
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Custom type string empty or null.");
        return false;
    }

    for (u32 i = 0; i < resource_system_state->config.max_loaders; i++)
    {
        ResourceLoader* loader = &resource_system_state->loaders[i];
        if (loader->id == INVALID_ID) continue;
        if (loader->type != RESOURCE_TYPE_CUSTOM) continue;
        
        // NOTE: see if case-insensitive is a good idea or not
        if (loader->custom_type != NULL && string_equals_nocase(loader->custom_type, custom_type))
        {
            return load_resource(name, loader, out_resource);
        }
    }

    out_resource->loader_id = INVALID_ID;
    log_error("Resource loader not found. [Custom type: %s]", custom_type);
    return false;
}

void resource_system_unload(Resource* resource)
{
    if (resource_system_state == NULL)
    {
        log_error("Resource system not initialized.");
        return;
    }

    if (resource->loader_id == INVALID_ID)
    {
        log_warning("Resource not loaded.");
        return;
    }

    ResourceLoader* loader = &resource_system_state->loaders[resource->loader_id];
    if (loader->id == INVALID_ID)
    {
        log_error("Resource loader not found. [ID: %d]", resource->loader_id);
        return;
    }
    if (loader->unload == NULL)
    {
        log_warning("Resource loader unload function not set. [ID: %d]", resource->loader_id);
        return;
    }   

    if (!loader->unload(loader, resource))
    {
        log_error("Resource unload failed. [ID: %d] [type: %d] [name: %s]", resource->loader_id, resource->type, resource->name);
    }
}

const char* resource_system_get_asset_base_path(void)
{
    if (resource_system_state == NULL)
    {
        log_error("Resource system not initialized.");
        return "";
    }

    return resource_system_state->config.asset_base_path;
}

bool load_resource(const char* name, ResourceLoader* loader, Resource* out_resource)
{
    if (!out_resource)
    {
        log_error("Resource output is null.");
        return false;
    }

    if (name == NULL || string_length(name) == 0)
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Resource name empty or null.");
        return false;
    }

    if (loader->load == NULL)
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Resource loader load function not set. [ID: %d] [type: %d]", loader->id, loader->type);
        return false;
    }

    out_resource->loader_id = loader->id;
    if (!loader->load(loader, name, out_resource))
    {
        out_resource->loader_id = INVALID_ID;
        log_error("Resource load failed. [ID: %d] [type: %d] [name: %s]", loader->id, loader->type, name);
        return false;
    }

    return true;
}