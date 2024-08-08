#include "vulkan_fence.h"
#include "core/log.h"

void vulkan_fence_create(VulkanContext* context, bool signaled, VulkanFence* out_fence)
{
    out_fence->signaled = signaled;
    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (out_fence->signaled)
    {
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    VK_ASSERT(vkCreateFence(context->device.logical_device, &fence_info, context->allocator, &out_fence->fence));
}

void vulkan_fence_destroy(VulkanContext* context, VulkanFence* fence)
{
    if (fence->fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(context->device.logical_device, fence->fence, context->allocator);
        fence->fence = VK_NULL_HANDLE;
    }
    fence->signaled = false;
}

bool vulkan_fence_wait(VulkanContext* context, VulkanFence* fence, u64 timeout)
{
    if (fence->signaled)
    {
        return true;
    }

    VkResult result = vkWaitForFences(context->device.logical_device, 1, &fence->fence, VK_TRUE, timeout);
    switch (result)
    {
        case VK_SUCCESS:
        {
            fence->signaled = true;
            return true;
        }
        case VK_TIMEOUT:
        {
            log_warning("vulkan_fence_wait: Fence wait timed out");
            break;
        }
        case VK_ERROR_DEVICE_LOST:
        {
            log_error("vulkan_fence_wait: Device lost");
            break;
        }
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        {
            log_error("vulkan_fence_wait: Out of host memory");
            break;
        }
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        {
            log_error("vulkan_fence_wait: Out of device memory");
            break;
        }
        default:
        {
            log_error("vulkan_fence_wait: Unknown error");
            break;
        }
    }

    return false;
}

void vulkan_fence_reset(VulkanContext* context, VulkanFence* fence)
{
    if (!fence->signaled)
    {
        return;
    }

    VK_ASSERT(vkResetFences(context->device.logical_device, 1, &fence->fence));
    fence->signaled = false;
}