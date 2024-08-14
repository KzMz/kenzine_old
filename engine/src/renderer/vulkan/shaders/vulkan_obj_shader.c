#include "vulkan_obj_shader.h"
#include "core/log.h"
#include "core/memory.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "lib/math/math_defines.h"
#include "../vulkan_pipeline.h"

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

bool vulkan_obj_shader_create(VulkanContext* context, VulkanObjShader* out_shader)
{
    char stage_type_str[OBJECT_SHADER_STAGE_COUNT][5] = {
        "vert",
        "frag"
    };
    VkShaderStageFlagBits stage_flags[OBJECT_SHADER_STAGE_COUNT] = {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT
    };

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
    {
        if (!create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_str[i], stage_flags[i], i, out_shader->stages))
        {
            log_error("Failed to create shader module for object shader");
            return false;
        }
    }

    VkViewport viewport;
    memory_zero(&viewport, sizeof(VkViewport));
    viewport.x = 0.0f;
    viewport.y = (f32) context->framebuffer_height;
    viewport.width = (f32) context->framebuffer_width;
    viewport.height = -(f32) context->framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    memory_zero(&scissor, sizeof(VkRect2D));
    scissor.offset = (VkOffset2D) {0, 0};
    scissor.extent = (VkExtent2D) {context->framebuffer_width, context->framebuffer_height};

    u32 attribute_offset = 0;
    const i32 attribute_count = 1;
    VkVertexInputAttributeDescription attributes[attribute_count];

    VkFormat format[attribute_count] = {
        VK_FORMAT_R32G32B32_SFLOAT
    };
    u64 sizes[attribute_count] = {
        sizeof(Vec3)
    };

    for (u32 i = 0; i < attribute_count; ++i)
    {
        attributes[i].binding = 0;
        attributes[i].location = i;
        attributes[i].format = format[i];
        attributes[i].offset = attribute_offset;
        attribute_offset += sizes[i];
    }

    VkPipelineShaderStageCreateInfo stages_create_infos[OBJECT_SHADER_STAGE_COUNT];
    memory_zero(stages_create_infos, sizeof(stages_create_infos));
    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; ++i)
    {
        stages_create_infos[i] = out_shader->stages[i].stage_info;
    }

    if (!vulkan_pipeline_create(
        context,
        &context->main_render_pass,
        attribute_count,
        attributes,
        0,
        NULL,
        OBJECT_SHADER_STAGE_COUNT,
        stages_create_infos,
        viewport,
        scissor,
        false,
        &out_shader->pipeline
    ))
    {
        log_error("Failed to create pipeline for object shader");
        return false;
    }

    return true;
}

void vulkan_obj_shader_destroy(VulkanContext* context, VulkanObjShader* shader)
{
    vulkan_pipeline_destroy(context, &shader->pipeline);

    for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
    {
        vkDestroyShaderModule(context->device.logical_device, shader->stages[i].module, context->allocator);
        shader->stages[i].module = VK_NULL_HANDLE;
    }
}

void vulkan_obj_shader_use(VulkanContext* context, VulkanObjShader* shader)
{
    u32 image_index = context->image_index;
    vulkan_pipeline_bind(&context->graphics_command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}