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

typedef enum VulkanRenderPassState
{
    VULKAN_RENDER_PASS_STATE_READY,
    VULKAN_RENDER_PASS_STATE_RECORDING,
    VULKAN_RENDER_PASS_STATE_IN_RENDER_PASS,
    VULKAN_RENDER_PASS_STATE_RECORDING_FINISHED,
    VULKAN_RENDER_PASS_STATE_SUBMITTED,
    VULKAN_RENDER_PASS_STATE_NOT_ALLOCATED
}  VulkanRenderPassState;

typedef struct VulkanRenderPass
{
    VkRenderPass render_pass;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
} VulkanRenderPass;

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

typedef enum VulkanCommandBufferState
{
    VULKAN_COMMAND_BUFFER_STATE_READY,
    VULKAN_COMMAND_BUFFER_STATE_RECORDING,
    VULKAN_COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    VULKAN_COMMAND_BUFFER_STATE_RECORDING_FINISHED,
    VULKAN_COMMAND_BUFFER_STATE_SUBMITTED,
    VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED
} VulkanCommandBufferState;

typedef struct VulkanCommandBuffer
{
    VkCommandBuffer command_buffer;
    VulkanCommandBufferState state;
} VulkanCommandBuffer;

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

    VulkanRenderPass main_render_pass;
} VulkanContext;