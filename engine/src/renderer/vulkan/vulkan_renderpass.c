#include "vulkan_renderpass.h"
#include "core/memory.h"

void vulkan_renderpass_create(VulkanContext* context, VulkanRenderPass* out_render_pass,
                              f32 x, f32 y, f32 w, f32 h,
                              f32 r, f32 g, f32 b, f32 a,
                              f32 depth,
                              u32 stencil)
{
    // Main subpass
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    u32 attachment_count = 2;
    VkAttachmentDescription attachments[attachment_count];

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = context->swapchain.image_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.flags = 0;

    attachments[0] = color_attachment;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkAttachmentDescription depth_attachment = {0};
    depth_attachment.format = context->device.depth_format;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments[1] = depth_attachment;

    VkAttachmentReference depth_attachment_ref = {0};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;

    subpass.pResolveAttachments = NULL;

    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_info.attachmentCount = attachment_count;
    render_pass_info.pAttachments = attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    render_pass_info.flags = 0;
    render_pass_info.pNext = NULL;

    VK_CHECK(vkCreateRenderPass(context->device.logical_device, &render_pass_info, context->allocator, &out_render_pass->render_pass));
}

void vulkan_renderpass_destroy(VulkanContext* context, VulkanRenderPass* render_pass)
{
    if (!render_pass || render_pass->render_pass == VK_NULL_HANDLE)
    {
        return;
    }

    vkDestroyRenderPass(context->device.logical_device, render_pass->render_pass, context->allocator);
    render_pass->render_pass = VK_NULL_HANDLE;
}

void vulkan_renderpass_begin(VulkanCommandBuffer* command_buffer, VulkanRenderPass* render_pass, VkFramebuffer frame_buffer)
{
    VkRenderPassBeginInfo render_pass_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_info.renderPass = render_pass->render_pass;
    render_pass_info.framebuffer = frame_buffer;
    render_pass_info.renderArea.offset.x = (i32) render_pass->x;
    render_pass_info.renderArea.offset.y = (i32) render_pass->y;
    render_pass_info.renderArea.extent.width = (u32) render_pass->w;
    render_pass_info.renderArea.extent.height = (u32) render_pass->h;

    VkClearValue clear_values[2];
    memory_zero(clear_values, sizeof(VkClearValue) * 2);
    clear_values[0].color = (VkClearColorValue) {
        .float32 = {
            render_pass->r, 
            render_pass->g, 
            render_pass->b, 
            render_pass->a
        }
    };
    clear_values[1].depthStencil = (VkClearDepthStencilValue) {
        .depth = render_pass->depth,
        .stencil = render_pass->stencil
    };

    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkan_renderpass_end(VulkanCommandBuffer* command_buffer, VulkanRenderPass* render_pass)
{
    vkCmdEndRenderPass(command_buffer->command_buffer);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}