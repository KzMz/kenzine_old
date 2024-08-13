#pragma once

#include "defines.h"

struct VulkanContext;

void platform_get_required_extension_names(const char*** names_dynarray);
bool platform_create_vulkan_surface(struct VulkanContext* context);