#pragma once

#include "vulkan_defines.h"

void vulkan_command_buffer_alloc(VulkanContext* context, VkCommandPool pool, bool is_primary, VulkanCommandBuffer* out_command_buffer);
void vulkan_command_buffer_free(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* command_buffer);

void vulkan_command_buffer_begin(VulkanCommandBuffer* command_buffer, bool is_single_use, bool is_simultaneous_use, bool is_renderpass_continue);
void vulkan_command_buffer_end(VulkanCommandBuffer* command_buffer);

void vulkan_command_buffer_update_submitted(VulkanCommandBuffer* command_buffer);
void vulkan_command_buffer_reset(VulkanCommandBuffer* command_buffer);

void vulkan_command_buffer_alloc_and_begin_single_use(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* out_command_buffer);
void vulkan_command_buffer_end_and_submit_single_use(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* command_buffer, VkQueue queue);