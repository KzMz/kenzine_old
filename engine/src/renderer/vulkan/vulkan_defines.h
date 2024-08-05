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

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat depth_format;
} VulkanDevice;

typedef struct VulkanImage 
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
} VulkanImage;

typedef struct VulkanSwapchain
{
    VkSurfaceFormatKHR image_format;
    u8 max_frames_in_flight;
    VkSwapchainKHR swapchain;
    u32 image_count;
    VkImage* images;
    VkImageView* image_views;

    VulkanImage depth_attachment;
} VulkanSwapchain;

typedef i32 (*VulkanFindMemoryIndex)(u32 type_filter, u32 property_flags);

typedef struct VulkanContext 
{
    u32 framebuffer_width;
    u32 framebuffer_height;

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface;

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

    VulkanDevice device;

    VulkanSwapchain swapchain;
    u32 image_index;
    u32 current_frame;

    bool recreating_swapchain;

    VulkanFindMemoryIndex find_memory_index;
} VulkanContext;