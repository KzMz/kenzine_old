#include "vulkan_renderpass.h"
#include "core/memory.h"

void vulkan_renderpass_create(VulkanContext* context, VulkanRenderPass* out_render_pass,
                              Vec4 render_area,
                              Vec4 clear_color,
                              f32 depth,
                              u32 stencil,
                              u8 clear_flags,
                              bool has_prev_pass,
                              bool has_next_pass)
{
    out_render_pass->render_area = render_area;
    out_render_pass->clear_color = clear_color;

    out_render_pass->depth = depth;
    out_render_pass->stencil = stencil;

    out_render_pass->clear_flags = clear_flags;
    out_render_pass->has_prev_pass = has_prev_pass;
    out_render_pass->has_next_pass = has_next_pass;

    // Main subpass
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    u32 attachment_count = 0;
    VkAttachmentDescription attachments[2];

    bool should_clear_color = (clear_flags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = context->swapchain.image_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = should_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = has_prev_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = has_next_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.flags = 0;

    attachments[attachment_count] = color_attachment;
    attachment_count++;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    bool should_clear_depth = (clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (should_clear_depth)
    {
        VkAttachmentDescription depth_attachment = {0};
        depth_attachment.format = context->device.depth_format;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = should_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[attachment_count] = depth_attachment;
        attachment_count++;

        VkAttachmentReference depth_attachment_ref = {0};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depth_attachment_ref;
    } 
    else
    {
        memory_zero(&attachments[attachment_count], sizeof(VkAttachmentDescription));
        subpass.pDepthStencilAttachment = NULL;
    }

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

    VK_ASSERT(vkCreateRenderPass(context->device.logical_device, &render_pass_info, context->allocator, &out_render_pass->render_pass));
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
    render_pass_info.renderArea.offset.x = (i32) render_pass->render_area.x;
    render_pass_info.renderArea.offset.y = (i32) render_pass->render_area.y;
    render_pass_info.renderArea.extent.width = (u32) render_pass->render_area.z;
    render_pass_info.renderArea.extent.height = (u32) render_pass->render_area.w;

    render_pass_info.clearValueCount = 0;
    render_pass_info.pClearValues = NULL;

    VkClearValue clear_values[2];
    memory_zero(clear_values, sizeof(VkClearValue) * 2);

    bool should_clear_color = (render_pass->clear_flags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;
    if (should_clear_color)
    {
        memory_copy(clear_values[render_pass_info.clearValueCount].color.float32, render_pass->clear_color.elements, sizeof(f32) * 4);
        render_pass_info.clearValueCount++;
    }

    bool should_clear_depth = (render_pass->clear_flags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (should_clear_depth)
    {
        memory_copy(clear_values[render_pass_info.clearValueCount].color.float32, &render_pass->clear_color.elements, sizeof(f32) * 4);
        clear_values[render_pass_info.clearValueCount].depthStencil.depth = render_pass->depth;

        bool should_clear_stencil = (render_pass->clear_flags & RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG) != 0;
        clear_values[render_pass_info.clearValueCount].depthStencil.stencil = should_clear_stencil ? render_pass->stencil : 0;
        render_pass_info.clearValueCount++;
    }

    render_pass_info.pClearValues = render_pass_info.clearValueCount > 0 ? clear_values : NULL;

    vkCmdBeginRenderPass(command_buffer->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkan_renderpass_end(VulkanCommandBuffer* command_buffer, VulkanRenderPass* render_pass)
{
    vkCmdEndRenderPass(command_buffer->command_buffer);
    command_buffer->state = VULKAN_COMMAND_BUFFER_STATE_RECORDING;
}