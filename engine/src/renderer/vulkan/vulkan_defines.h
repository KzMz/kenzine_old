#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>
#include "core/asserts.h"

#define VK_CHECK(expr) do { kz_assert((expr) == VK_SUCCESS); } while(0)

typedef struct VulkanSwapchainSupportInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    u32 format_count;
    VkPresentModeKHR* present_modes;
    u32 present_mode_count;
} VulkanSwapchainSupportInfo;

typedef struct VulkanDevice
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    VulkanSwapchainSupportInfo swapchain_support;
    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;
} VulkanDevice;

typedef struct VulkanContext 
{
    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    VulkanDevice device;
} VulkanContext;