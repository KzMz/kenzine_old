#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

#include "core/log.h"
#include "core/memory.h"

bool vulkan_buffer_create(
    VulkanContext* context,
    u64 size,
    VkBufferUsageFlagBits usage,
    u32 memory_property_flags,
    bool bind,
    VulkanBuffer* out_buffer
)
{
    memory_zero(out_buffer, sizeof(VulkanBuffer));
    out_buffer->size = size;
    out_buffer->usage = usage;
    out_buffer->memory_property_flags = memory_property_flags;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_ASSERT(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &out_buffer->buffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device.logical_device, out_buffer->buffer, &requirements);
    out_buffer->memory_index = context->find_memory_index(requirements.memoryTypeBits, out_buffer->memory_property_flags);
    if (out_buffer->memory_index < 0)
    {
        log_error("Failed to find memory index for buffer");
        return false;
    }

    VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = out_buffer->memory_index;

    VK_ASSERT(vkAllocateMemory(context->device.logical_device, &alloc_info, context->allocator, &out_buffer->memory));

    if (bind)
    {
        vulkan_buffer_bind(context, out_buffer, 0);
    }

    return true;
}

void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer)
{
    if (buffer->memory)
    {
        vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);
        buffer->memory = VK_NULL_HANDLE;
    }
    if (buffer->buffer)
    {
        vkDestroyBuffer(context->device.logical_device, buffer->buffer, context->allocator);
        buffer->buffer = VK_NULL_HANDLE;
    }

    buffer->size = 0;
    buffer->locked = false;
    buffer->usage = 0;
}

bool vulkan_buffer_resize(
    VulkanContext* context,
    u64 new_size,
    VulkanBuffer* buffer,
    VkQueue queue,
    VkCommandPool pool
)
{
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = new_size;
    buffer_info.usage = buffer->usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer new_buffer;
    VK_ASSERT(vkCreateBuffer(context->device.logical_device, &buffer_info, context->allocator, &new_buffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(context->device.logical_device, new_buffer, &requirements);

    VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = buffer->memory_index;

    VkDeviceMemory new_memory;
    VK_ASSERT(vkAllocateMemory(context->device.logical_device, &alloc_info, context->allocator, &new_memory));

    VK_ASSERT(vkBindBufferMemory(context->device.logical_device, new_buffer, new_memory, 0));  

    vulkan_buffer_copy(context, pool, VK_NULL_HANDLE, queue, buffer->buffer, 0, new_buffer, 0, buffer->size);  

    vkDeviceWaitIdle(context->device.logical_device);

    if (buffer->memory)
    {
        vkFreeMemory(context->device.logical_device, buffer->memory, context->allocator);
        buffer->memory = VK_NULL_HANDLE;
    }
    if (buffer->buffer)
    {
        vkDestroyBuffer(context->device.logical_device, buffer->buffer, context->allocator);
        buffer->buffer = VK_NULL_HANDLE;
    }

    buffer->buffer = new_buffer;
    buffer->memory = new_memory;
    buffer->size = new_size;

    return true;
}

void vulkan_buffer_bind(VulkanContext* context, VulkanBuffer* buffer, u64 offset)
{
    VK_ASSERT(vkBindBufferMemory(context->device.logical_device, buffer->buffer, buffer->memory, offset));
}

void* vulkan_buffer_lock(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags)
{
    void* data;
    VK_ASSERT(vkMapMemory(context->device.logical_device, buffer->memory, offset, size, flags, &data));
    return data;
}

void vulkan_buffer_unlock(VulkanContext* context, VulkanBuffer* buffer)
{
    vkUnmapMemory(context->device.logical_device, buffer->memory);
}

void vulkan_buffer_load_data(VulkanContext* context, VulkanBuffer* buffer, u64 offset, u64 size, u32 flags, const void* data)
{
    void* mapped_data = NULL;
    VK_ASSERT(vkMapMemory(context->device.logical_device, buffer->memory, offset, size, flags, &mapped_data));
    memory_copy(mapped_data, data, size);
    vkUnmapMemory(context->device.logical_device, buffer->memory);
}

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
)
{
    vkQueueWaitIdle(queue);

    VulkanCommandBuffer command_buffer;
    vulkan_command_buffer_alloc_and_begin_single_use(context, pool, &command_buffer);

    VkBufferCopy copy_region = {source_offset, destination_offset, size};
    vkCmdCopyBuffer(command_buffer.command_buffer, source, destination, 1, &copy_region);

    vulkan_command_buffer_end_and_submit_single_use(context, pool, &command_buffer, queue);
}