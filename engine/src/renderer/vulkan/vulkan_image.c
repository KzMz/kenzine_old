#include "vulkan_image.h"
#include "vulkan_device.h"
#include "core/log.h"
#include "core/memory.h"

void vulkan_image_create(
    VulkanContext* context,
    VkImageType image_type,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memory_flags,
    bool create_view,
    VkImageAspectFlags view_aspect_flags,
    VulkanImage* out_image
)
{
    out_image->width = width;
    out_image->height = height;

    VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = image_type;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 4;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_ASSERT(vkCreateImage(context->device.logical_device, &image_info, context->allocator, &out_image->image));
    
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(context->device.logical_device, out_image->image, &memory_requirements);

    i32 memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);
    if (memory_type == -1)
    {
        log_fatal("Failed to find suitable memory type.");
    }

    VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = memory_type;

    VK_ASSERT(vkAllocateMemory(context->device.logical_device, &alloc_info, context->allocator, &out_image->memory));

    VK_ASSERT(vkBindImageMemory(context->device.logical_device, out_image->image, out_image->memory, 0));

    if (create_view)
    {
        out_image->view = VK_NULL_HANDLE;
        vulkan_image_view_create(context, format, out_image, view_aspect_flags);
    }
}

void vulkan_image_view_create(
    VulkanContext* context,
    VkFormat format,
    VulkanImage* image,
    VkImageAspectFlags aspect_flags
)
{
    VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = image->image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;

    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VK_ASSERT(vkCreateImageView(context->device.logical_device, &view_info, context->allocator, &image->view));
}

void vulkan_image_destroy(VulkanContext* context, VulkanImage* image)
{
    if (image->view)
    {
        vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
        image->view = VK_NULL_HANDLE;
    }

    if (image->memory)
    {
        vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
        image->memory = VK_NULL_HANDLE;
    }

    if (image->image)
    {
        vkDestroyImage(context->device.logical_device, image->image, context->allocator);
        image->image = VK_NULL_HANDLE;
    }
}