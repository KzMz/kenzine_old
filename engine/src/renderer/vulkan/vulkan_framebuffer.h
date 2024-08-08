#pragma once

#include "vulkan_defines.h"

void vulkan_framebuffer_create(
    VulkanContext* context, VulkanRenderPass* render_pass,
    u32 width, u32 height,
    u32 attachment_count, VkImageView* attachments,
    VulkanFramebuffer* out_framebuffer);

void vulkan_framebuffer_destroy(VulkanContext* context, VulkanFramebuffer* framebuffer);