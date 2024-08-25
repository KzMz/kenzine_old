#include "vulkan_buffer.h"

#include "vulkan_device.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"

#include "core/log.h"
#include "core/memory.h"

#include "lib/memory/freelist.h"

void freelist_cleanup(VulkanBuffer* buffer);

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

    u64 nodes_size = freelist_get_nodes_size(size);
    out_buffer->freelist_memory = memory_alloc_c(nodes_size, MEMORY_ALLOCATION_TYPE_DYNAMIC, MEMORY_TAG_RENDERER);
    if (out_buffer->freelist_memory == NULL)
    {
        log_error("Failed to allocate memory for freelist");
        return false;
    }
    out_buffer->free_list_size = nodes_size;
    freelist_create(size, out_buffer->freelist_memory, &out_buffer->free_list);

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
        freelist_cleanup(out_buffer);
        return false;
    }

    VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = out_buffer->memory_index;

    VkResult result = vkAllocateMemory(context->device.logical_device, &alloc_info, context->allocator, &out_buffer->memory);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to allocate memory for buffer");
        freelist_cleanup(out_buffer);
        return false;
    }

    if (bind)
    {
        vulkan_buffer_bind(context, out_buffer, 0);
    }

    return true;
}

void vulkan_buffer_destroy(VulkanContext* context, VulkanBuffer* buffer)
{
    if (buffer->freelist_memory != NULL)
    {
        freelist_cleanup(buffer);
    }
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
    if (new_size < buffer->size)
    {
        log_error("New size must be greater than current size");
        return false;
    }

    u64 nodes_size = freelist_get_nodes_size(new_size);
    void* old_block = NULL;
    void* new_block = memory_alloc_c(nodes_size, MEMORY_ALLOCATION_TYPE_DYNAMIC, MEMORY_TAG_RENDERER);
    if (!freelist_resize(&buffer->free_list, new_size, new_block, &old_block))
    {
        log_error("Failed to resize freelist");
        memory_free_c(new_block, nodes_size, MEMORY_ALLOCATION_TYPE_DYNAMIC, MEMORY_TAG_RENDERER);
        return false;
    }
    memory_free_c(old_block, buffer->free_list_size, MEMORY_ALLOCATION_TYPE_DYNAMIC, MEMORY_TAG_RENDERER);
    buffer->free_list_size = nodes_size;
    buffer->freelist_memory = new_block;
    buffer->size = new_size;

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

bool vulkan_buffer_alloc(VulkanBuffer* buffer, u64 size, u64* out_offset)
{
    if (buffer == NULL)
    {
        log_error("VulkanBuffer buffer must not be NULL");
        return false;
    }
    if (size == 0)
    {
        log_error("VulkanBuffer requested size must be greater than 0");
        return false;
    }
    if (out_offset == NULL)
    {
        log_error("VulkanBuffer out_offset must not be NULL");
        return false;
    }

    return freelist_alloc(&buffer->free_list, size, out_offset);
}

bool vulkan_buffer_free(VulkanBuffer* buffer, u64 size, u64 offset)
{
    if (buffer == NULL)
    {
        log_error("VulkanBuffer buffer must not be NULL");
        return false;
    }
    if (size == 0)
    {
        log_error("VulkanBuffer requested size must be greater than 0");
        return false;
    }

    return freelist_free(&buffer->free_list, size, offset);
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

void freelist_cleanup(VulkanBuffer* buffer)
{
    freelist_destroy(&buffer->free_list);
    memory_free_c(buffer->freelist_memory, buffer->free_list_size, MEMORY_ALLOCATION_TYPE_DYNAMIC, MEMORY_TAG_RENDERER);
    buffer->freelist_memory = NULL;
    buffer->free_list_size = 0;
}