#pragma once

#include "defines.h"
#include "resources/resource_defines.h"

struct JsonNode;

KENZINE_API bool json_utils_get_resource_metadata(
    ResourceType type, 
    struct JsonNode* root, 
    u32 resource_name_size, 
    ResourceMetadata* out_metadata
);