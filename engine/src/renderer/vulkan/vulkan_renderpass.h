#pragma once

#include "vulkan_defines.h"
#include "lib/math/math_defines.h"

typedef enum RenderPassClearFlag
{
    RENDERPASS_CLEAR_NONE_FLAG = 0x00,
    RENDERPASS_CLEAR_COLOR_BUFFER_FLAG = 0x01,
    RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG = 0x02,
    RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG = 0x04
} RenderPassClearFlag;

void vulkan_renderpass_create(VulkanContext* context, VulkanRenderPass* out_render_pass,
                              Vec4 render_area,
                              Vec4 clear_color,
                              f32 depth,
                              u32 stencil,
                              u8 clear_flags,
                              bool has_prev_pass,
                              bool has_next_pass);

void vulkan_renderpass_destroy(VulkanContext* context, VulkanRenderPass* render_pass);

void vulkan_renderpass_begin(VulkanCommandBuffer* command_buffer, VulkanRenderPass* render_pass, VkFramebuffer frame_buffer);
void vulkan_renderpass_end(VulkanCommandBuffer* command_buffer, VulkanRenderPass* render_pass);