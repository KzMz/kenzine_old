#include "vulkan_pipeline.h"
#include "vulkan_utils.h"

#include "core/log.h"
#include "core/memory.h"

#include "lib/math/math_defines.h"

bool vulkan_pipeline_create(
    VulkanContext* context,
    VulkanRenderPass* render_pass,
    u32 attribute_count,
    VkVertexInputAttributeDescription* attributes,
    u32 descriptor_count,
    VkDescriptorSetLayout* descriptor_layouts,
    u32 stage_count,
    VkPipelineShaderStageCreateInfo* stages,
    VkViewport viewport,
    VkRect2D scissor,
    bool is_wireframe,
    VulkanPipeline* out_pipeline
)
{
    VkPipelineViewportStateCreateInfo viewport_state = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterization_state.lineWidth = 1.0f;
    rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f;
    rasterization_state.depthBiasClamp = 0.0f;
    rasterization_state.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = NULL;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth_stencil_state.depthTestEnable = VK_TRUE;
    depth_stencil_state.depthWriteEnable = VK_TRUE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    memory_zero(&color_blend_attachment, sizeof(VkPipelineColorBlendAttachmentState));
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_blend_attachment;

    const u32 dynamic_state_count = 3;
    VkDynamicState dynamic_states[dynamic_state_count] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamic_state = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state.dynamicStateCount = dynamic_state_count;
    dynamic_state.pDynamicStates = dynamic_states;

    VkVertexInputBindingDescription binding_description = {0};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex3d);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state.vertexBindingDescriptionCount = 1;
    vertex_input_state.pVertexBindingDescriptions = &binding_description;
    vertex_input_state.vertexAttributeDescriptionCount = attribute_count;
    vertex_input_state.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    VkPushConstantRange push_constant_range = {0};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset = sizeof(Mat4) * 0;
    push_constant_range.size = sizeof(Mat4) * 2;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    pipeline_layout_info.setLayoutCount = descriptor_count;
    pipeline_layout_info.pSetLayouts = descriptor_layouts;

    VK_ASSERT(vkCreatePipelineLayout(
        context->device.logical_device, &pipeline_layout_info, context->allocator, &out_pipeline->layout));

    VkGraphicsPipelineCreateInfo pipeline_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.stageCount = stage_count;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input_state;
    pipeline_info.pInputAssemblyState = &input_assembly_state;

    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization_state;
    pipeline_info.pMultisampleState = &multisample_state;
    pipeline_info.pDepthStencilState = &depth_stencil_state;
    pipeline_info.pColorBlendState = &color_blend_state;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.pTessellationState = NULL;

    pipeline_info.layout = out_pipeline->layout;

    pipeline_info.renderPass = render_pass->render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(
        context->device.logical_device, VK_NULL_HANDLE, 1, &pipeline_info, context->allocator, &out_pipeline->pipeline);

    if (!vulkan_result_is_successful(result))
    {
        log_error("Failed to create graphics pipeline. %s", vulkan_result_string(result, true));
        return false;
    }

    log_debug("Successfully created graphics pipeline");
    return true;
}

void vulkan_pipeline_destroy(
    VulkanContext* context,
    VulkanPipeline* pipeline
)
{
    if (!pipeline) 
    {
        return;
    }

    if (pipeline->pipeline)
    {
        vkDestroyPipeline(context->device.logical_device, pipeline->pipeline, context->allocator);
        pipeline->pipeline = VK_NULL_HANDLE;
    }

    if (pipeline->layout)
    {
        vkDestroyPipelineLayout(context->device.logical_device, pipeline->layout, context->allocator);
        pipeline->layout = VK_NULL_HANDLE;
    }
}

void vulkan_pipeline_bind(
    VulkanCommandBuffer* command_buffer,
    VkPipelineBindPoint bind_point,
    VulkanPipeline* pipeline
)
{
    vkCmdBindPipeline(command_buffer->command_buffer, bind_point, pipeline->pipeline);
}