#include "vulkan_framebuffer.h"
#include "core/memory.h"

void vulkan_framebuffer_create(
    VulkanContext* context, VulkanRenderPass* render_pass,
    u32 width, u32 height,
    u32 attachment_count, VkImageView* attachments,
    VulkanFramebuffer* out_framebuffer)
{
    out_framebuffer->attachments = memory_alloc(sizeof(VkImageView) * attachment_count, MEMORY_TAG_RENDERER);
    for (u32 i = 0; i < attachment_count; ++i)
    {
        out_framebuffer->attachments[i] = attachments[i];
    }

    out_framebuffer->attachment_count = attachment_count;
    out_framebuffer->render_pass = render_pass;

    VkFramebufferCreateInfo framebuffer_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebuffer_info.renderPass = render_pass->render_pass;
    framebuffer_info.attachmentCount = attachment_count;
    framebuffer_info.pAttachments = out_framebuffer->attachments;
    framebuffer_info.width = width;
    framebuffer_info.height = height;
    framebuffer_info.layers = 1;

    VK_CHECK(vkCreateFramebuffer(context->device.logical_device, &framebuffer_info, context->allocator, &out_framebuffer->framebuffer));
}

void vulkan_framebuffer_destroy(VulkanContext* context, VulkanFramebuffer* framebuffer)
{
    vkDestroyFramebuffer(context->device.logical_device, framebuffer->framebuffer, context->allocator);
    if (framebuffer->attachments)
    {
        memory_free(framebuffer->attachments, sizeof(VkImageView) * framebuffer->attachment_count, MEMORY_TAG_RENDERER);
        framebuffer->attachments = NULL;
    }
    
    framebuffer->attachment_count = 0;
    framebuffer->render_pass = NULL;
    framebuffer->framebuffer = VK_NULL_HANDLE;
}