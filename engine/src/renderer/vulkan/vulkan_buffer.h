#pragma once

#include "vulkan_defines.h"

bool vulkan_buffer_create(
    VulkanContext* context,
    u64 size,
    VkBufferUsageFlagBits usage,
    u32 memory_property_flags,
    bool bind,
    bool has_freelist,
    VulkanBuffer* out_buffer
);

void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer);

bool vulkan_buffer_resize(
    VulkanContext* context,
    u64 new_size,
    VulkanBuffer* buffer,
    VkQueue queue,
    VkCommandPool pool
);


void vulkan_buffer_bind(VulkanContext* context, VulkanBuffer* buffer, u64 offset);
void* vulkan_buffer_lock(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags);
void vulkan_buffer_unlock(VulkanContext* context, VulkanBuffer* buffer);
void vulkan_buffer_load_data(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags, const void* data);

bool vulkan_buffer_alloc(VulkanBuffer* buffer, u64 size, u64* out_offset);
bool vulkan_buffer_free(VulkanBuffer* buffer, u64 size, u64 offset);

void vulkan_buffer_copy(
    VulkanContext* context,
    VkCommandPool pool,
    VkFence fence,
    VkQueue queue,
    VkBuffer source,
    u64 source_offset,
    VkBuffer destination,
    u64 destination_offset,
    u64 size
);