#pragma once

#include "resources/resource_defines.h"

struct ResourceLoader;

typedef bool (*ResourceLoad)(struct ResourceLoader* self, const char* name, Resource* out_resource);
typedef bool (*ResourceUnload)(struct ResourceLoader* self, Resource* resource);

typedef struct ResourceSystemConfig
{
    u32 max_loaders;
    char* asset_base_path;
} ResourceSystemConfig;

typedef struct ResourceLoader
{
    u64 id;
    ResourceType type;
    const char* custom_type;
    const char* type_path;

    ResourceLoad load;
    ResourceUnload unload;
} ResourceLoader;

bool resource_system_init(void* state, ResourceSystemConfig config);
void resource_system_shutdown(void);
u64 resource_system_get_state_size(ResourceSystemConfig config);

KENZINE_API bool resource_system_register_loader(ResourceLoader loader);

KENZINE_API bool resource_system_load(const char* name, ResourceType type, Resource* out_resource);
KENZINE_API bool resource_system_load_custom(const char* name, const char* custom_type, Resource* out_resource);

KENZINE_API void resource_system_unload(Resource* resource);

KENZINE_API const char* resource_system_get_asset_base_path(void);