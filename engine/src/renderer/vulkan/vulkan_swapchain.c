#include "vulkan_swapchain.h"
#include "vulkan_device.h"
#include "core/log.h"
#include "core/memory.h"
#include "vulkan_image.h"

void create(VulkanContext* context, u32 width, u32 height, VulkanSwapchain* swapchain);
void destroy(VulkanContext* context, VulkanSwapchain* swapchain);

void vulkan_swapchain_create(VulkanContext* context, u32 width, u32 height, VulkanSwapchain* swapchain)
{
    create(context, width, height, swapchain);
}

void vulkan_swapchain_recreate(VulkanContext* context, u32 width, u32 height, VulkanSwapchain* swapchain)
{
    destroy(context, swapchain);
    create(context, width, height, swapchain);
}

void vulkan_swapchain_destroy(VulkanContext* context, VulkanSwapchain* swapchain)
{
    destroy(context, swapchain);
}

bool vulkan_swapchain_acquire_next_image(
    VulkanContext* context, 
    VulkanSwapchain* swapchain, 
    u64 timeout,
    VkSemaphore image_available_semaphore,
    VkFence fence,
    u32* out_image_index)
{
    VkResult result = vkAcquireNextImageKHR(
        context->device.logical_device, 
        swapchain->swapchain, 
        timeout, 
        image_available_semaphore, 
        fence, 
        out_image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
        return false;
    } 
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        log_fatal("Failed to acquire next image.");
        return false;
    }

    return true;
}

void vulkan_swapchain_present(
    VulkanContext* context, 
    VulkanSwapchain* swapchain, 
    VkQueue graphics_queue,
    VkQueue present_queue,
    VkSemaphore render_complete_semaphore,
    u32 present_image_index)
{
    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->swapchain;
    present_info.pImageIndices = &present_image_index;
    present_info.pResults = NULL;

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        vulkan_swapchain_recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
    }
    else if (result != VK_SUCCESS)
    {
        log_fatal("Failed to present image.");
    }

    context->current_frame = (context->current_frame + 1) % swapchain->max_frames_in_flight;
}

void create(VulkanContext* context, u32 width, u32 height, VulkanSwapchain* swapchain)
{
    VkExtent2D extent = {width, height};

    bool found = false;
    for (u32 i = 0; i < context->device.swapchain_support.format_count; ++i)
    {
        VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain->image_format = format;
            found = true;
            break;
        }
    }

    if (!found)
    {
        swapchain->image_format = context->device.swapchain_support.formats[0];
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < context->device.swapchain_support.present_mode_count; ++i)
    {
        VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = mode;
            break;
        }
    }

    vulkan_device_query_swapchain_support(
        context->device.physical_device, 
        context->surface, 
        &context->device.swapchain_support);

    if (context->device.swapchain_support.capabilities.currentExtent.width != 0xffffffff)
    {
        extent = context->device.swapchain_support.capabilities.currentExtent;
    }

    VkExtent2D min = context->device.swapchain_support.capabilities.minImageExtent;
    VkExtent2D max = context->device.swapchain_support.capabilities.maxImageExtent;
    extent.width = kz_clamp(extent.width, min.width, max.width);
    extent.height = kz_clamp(extent.height, min.height, max.height);

    u32 image_count = context->device.swapchain_support.capabilities.minImageCount + 1;
    if (context->device.swapchain_support.capabilities.maxImageCount > 0 && image_count > context->device.swapchain_support.capabilities.maxImageCount)
    {
        image_count = context->device.swapchain_support.capabilities.maxImageCount;
    }

    swapchain->max_frames_in_flight = image_count - 1;

    VkSwapchainCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create_info.surface = context->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = swapchain->image_format.format;
    create_info.imageColorSpace = swapchain->image_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (context->device.graphics_queue_index != context->device.present_queue_index)
    {
        u32 indices[] = {context->device.graphics_queue_index, context->device.present_queue_index};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }

    create_info.preTransform = context->device.swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_ASSERT(vkCreateSwapchainKHR(context->device.logical_device, &create_info, context->allocator, &swapchain->swapchain));

    context->current_frame = 0;
    swapchain->image_count = 0;
    VK_ASSERT(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->swapchain, &swapchain->image_count, NULL));
    if (!swapchain->images) 
    {
        swapchain->images = memory_alloc(sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    if (!swapchain->image_views)
    {
        swapchain->image_views = memory_alloc(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    VK_ASSERT(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain->swapchain, &swapchain->image_count, swapchain->images));

    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain->image_format.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(context->device.logical_device, &view_info, context->allocator, &swapchain->image_views[i]));
    }

    if (!vulkan_device_detect_depth_format(&context->device))
    {
        context->device.depth_format = VK_FORMAT_UNDEFINED;
        log_fatal("Failed to detect depth format.");
    }

    vulkan_image_create(
        context,
        VK_IMAGE_TYPE_2D,
        extent.width,
        extent.height,
        context->device.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depth_attachment);

    log_info("Swapchain created.");
}

void destroy(VulkanContext* context, VulkanSwapchain* swapchain)
{
    vkDeviceWaitIdle(context->device.logical_device);

    vulkan_image_destroy(context, &swapchain->depth_attachment);

    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        vkDestroyImageView(context->device.logical_device, swapchain->image_views[i], context->allocator);
    }

    vkDestroySwapchainKHR(context->device.logical_device, swapchain->swapchain, context->allocator);
}