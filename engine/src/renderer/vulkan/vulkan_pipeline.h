#pragma once

#include "vulkan_defines.h"

bool vulkan_pipeline_create(
    VulkanContext* context,
    VulkanRenderPass* render_pass,
    u32 stride,
    u32 attribute_count,
    VkVertexInputAttributeDescription* attributes,
    u32 descriptor_count,
    VkDescriptorSetLayout* descriptor_layouts,
    u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    bool is_wireframe,
    bool use_depth_test,
    u32 push_constant_range_count,
    Range* push_constant_ranges,
    VulkanPipeline* out_pipeline
);

void vulkan_pipeline_destroy(
    VulkanContext* context,
    VulkanPipeline* pipeline
);

void vulkan_pipeline_bind(
    VulkanCommandBuffer* command_buffer,
    VkPipelineBindPoint bind_point,
    VulkanPipeline* pipeline
);