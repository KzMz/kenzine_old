#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>
#include "core/asserts.h"

#define VK_CHECK(expr) do { kz_assert((expr) == VK_SUCCESS); } while(0)

typedef struct VulkanContext 
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

} VulkanContext;