#pragma once

#include "vulkan_defines.h"

void vulkan_image_create(
    VulkanContext* context,
    VkImageType image_type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    bool create_view,
    VkImageAspectFlags view_aspect_flags,
    VulkanImage* image
);

void vulkan_image_view_create(
    VulkanContext* context,
    VkFormat format,
    VulkanImage* image,
    VkImageAspectFlags aspect_flags
);

void vulkan_image_transition_layout(
    VulkanContext* context,
    VulkanCommandBuffer* command_buffer,
    VulkanImage* image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout
);

void vulkan_image_copy_from_buffer(
    VulkanContext* context,
    VulkanImage* image,
    VkBuffer buffer,
    VulkanCommandBuffer* command_buffer
);

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image);