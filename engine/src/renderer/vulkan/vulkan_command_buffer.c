#include "vulkan_command_buffer.h"
#include "core/memory.h"

void vulkan_command_buffer_alloc(VulkanContext* context, VkCommandPool pool, bool is_primary, VulkanCommandBuffer* out_command_buffer)
{
    memory_zero(out_command_buffer, sizeof(VulkanCommandBuffer));

    VkCommandBufferAllocateInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_info.commandPool = pool;
    command_buffer_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    command_buffer_info.commandBufferCount = 1;
    command_buffer_info.pNext = NULL;

    out_command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED;

    VK_CHECK(vkAllocateCommandBuffers(context->device.logical_device, &command_buffer_info, &out_command_buffer->command_buffer));

    out_command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_free(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* command_buffer)
{
    if (command_buffer->state == VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED)
    {
        return;
    }

    vkFreeCommandBuffers(context->device.logical_device, pool, 1, &command_buffer->command_buffer);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    command_buffer->command_buffer = VK_NULL_HANDLE;
}

void vulkan_command_buffer_begin(VulkanCommandBuffer* command_buffer, bool is_single_use, bool is_simultaneous_use, bool is_renderpass_continue)
{
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.pInheritanceInfo = NULL;
    begin_info.pNext = NULL;
    begin_info.flags = 0;

    if (is_single_use)
    {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    if (is_simultaneous_use)
    {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    if (is_renderpass_continue)
    {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }

    VK_CHECK(vkBeginCommandBuffer(command_buffer->command_buffer, &begin_info));
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}

void vulkan_command_buffer_end(VulkanCommandBuffer* command_buffer)
{
    if (command_buffer->state != VULKAN_COMMAND_BUFFER_STATE_RECORDING)
    {
        return;
    }

    VK_CHECK(vkEndCommandBuffer(command_buffer->command_buffer));
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING_FINISHED;
}

void vulkan_command_buffer_update_submitted(VulkanCommandBuffer* command_buffer)
{
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkan_command_buffer_reset(VulkanCommandBuffer* command_buffer)
{
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_alloc_and_begin_single_use(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* out_command_buffer)
{
    vulkan_command_buffer_alloc(context, pool, true, out_command_buffer);
    vulkan_command_buffer_begin(out_command_buffer, true, false, false);
}

void vulkan_command_buffer_end_and_submit_single_use(VulkanContext* context, VkCommandPool pool, VulkanCommandBuffer* command_buffer, VkQueue queue)
{
    vulkan_command_buffer_end(command_buffer);

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->command_buffer;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

    VK_CHECK(vkQueueWaitIdle(queue));

    vulkan_command_buffer_free(context, pool, command_buffer);
}