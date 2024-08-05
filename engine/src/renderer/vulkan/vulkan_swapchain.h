#pragma once

#include "vulkan_defines.h"

void vulkan_swapchain_create(VulkanContext* context, u32 width, u32 height, VulkanSwapchain* swapchain);
void vulkan_swapchain_recreate(VulkanContext* context, u32 width, u32 height, VulkanSwapchain* swapchain);
void vulkan_swapchain_destroy(VulkanContext* context, VulkanSwapchain* swapchain);
bool vulkan_swapchain_acquire_next_image(
    VulkanContext* context, 
    VulkanSwapchain* swapchain, 
    u64 timeout,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* out_image_index);
void vulkan_swapchain_present(
    VulkanContext* context, 
    VulkanSwapchain* swapchain, 
    VkQueue graphics_queue,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 present_image_index);