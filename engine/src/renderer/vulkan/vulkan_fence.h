#pragma once

#include "vulkan_defines.h"

void vulkan_fence_create(VulkanContext* context, bool signaled, VulkanFence* out_fence);
void vulkan_fence_destroy(VulkanContext* context, VulkanFence* fence);
bool vulkan_fence_wait(VulkanContext* context, VulkanFence* fence, u64 timeout);
void vulkan_fence_reset(VulkanContext* context, VulkanFence* fence);