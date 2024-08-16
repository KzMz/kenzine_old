#include "vulkan_obj_shader.h"
#include "core/log.h"
#include "core/memory.h"
#include "renderer/vulkan/vulkan_shader_utils.h"
#include "lib/math/math_defines.h"
#include "lib/math/math.h"
#include "../vulkan_pipeline.h"
#include "../vulkan_buffer.h"   

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.ObjectShader"

bool vulkan_obj_shader_create(VulkanContext* context, Texture* default_diffuse, VulkanObjShader* out_shader)
{
    out_shader->default_diffuse = default_diffuse;

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

    // Global
    VkDescriptorSetLayoutBinding global_uniform_binding;
    global_uniform_binding.binding = 0;
    global_uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_uniform_binding.descriptorCount = 1;
    global_uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    global_uniform_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor_layout_info.bindingCount = 1;
    descriptor_layout_info.pBindings = &global_uniform_binding;
    VK_ASSERT(vkCreateDescriptorSetLayout(
        context->device.logical_device, &descriptor_layout_info, context->allocator, &out_shader->global_descriptor_set_layout));

    VkDescriptorPoolSize pool_size;
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = context->swapchain.image_count;

    VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = context->swapchain.image_count;
    VK_ASSERT(vkCreateDescriptorPool(context->device.logical_device, &pool_info, context->allocator, &out_shader->global_descriptor_pool));

    // Local
    const u32 local_sampler_count = 1;
    VkDescriptorType descriptor_types[OBJECT_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
    };
    VkDescriptorSetLayoutBinding bindings[OBJECT_SHADER_DESCRIPTOR_COUNT] = {0};
    for (u32 i = 0; i < OBJECT_SHADER_DESCRIPTOR_COUNT; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptor_types[i];
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[i].pImmutableSamplers = NULL;
    } 

    VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = OBJECT_SHADER_DESCRIPTOR_COUNT;
    layout_info.pBindings = bindings;
    VK_ASSERT(vkCreateDescriptorSetLayout(context->device.logical_device, &layout_info, context->allocator, &out_shader->local_descriptor_set_layout));

    VkDescriptorPoolSize local_pool_sizes[OBJECT_SHADER_DESCRIPTOR_COUNT];
    local_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    local_pool_sizes[0].descriptorCount = MAX_OBJECT_COUNT;
    local_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    local_pool_sizes[1].descriptorCount = local_sampler_count * MAX_OBJECT_COUNT;

    VkDescriptorPoolCreateInfo local_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    local_pool_info.poolSizeCount = 1;
    local_pool_info.pPoolSizes = local_pool_sizes;
    local_pool_info.maxSets = MAX_OBJECT_COUNT;

    VK_ASSERT(vkCreateDescriptorPool(context->device.logical_device, &local_pool_info, context->allocator, &out_shader->local_descriptor_pool));

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

    // Attributes
    u32 attribute_offset = 0;
    const i32 attribute_count = 2;
    VkVertexInputAttributeDescription attributes[attribute_count];

    VkFormat format[attribute_count] = {
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT
    };
    u64 sizes[attribute_count] = {
        sizeof(Vec3),
        sizeof(Vec2)
    };

    for (u32 i = 0; i < attribute_count; ++i)
    {
        attributes[i].binding = 0;
        attributes[i].location = i;
        attributes[i].format = format[i];
        attributes[i].offset = attribute_offset;
        attribute_offset += sizes[i];
    }

    const i32 descriptor_count = 2;
    VkDescriptorSetLayout layouts[descriptor_count] = {out_shader->global_descriptor_set_layout, out_shader->local_descriptor_set_layout};

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
        descriptor_count,
        layouts,
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

    if (!vulkan_buffer_create(
        context,
        sizeof(GlobalUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true,
        &out_shader->global_uniform_buffer
    ))
    {
        log_error("Failed to create global uniform buffer for object shader");
        return false;
    }

    VkDescriptorSetLayout global_layouts[3] = 
    {
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout,
        out_shader->global_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo descriptor_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptor_alloc_info.descriptorPool = out_shader->global_descriptor_pool;
    descriptor_alloc_info.descriptorSetCount = 3;
    descriptor_alloc_info.pSetLayouts = global_layouts;
    VK_ASSERT(vkAllocateDescriptorSets(context->device.logical_device, &descriptor_alloc_info, out_shader->global_descriptor_set));

    if (!vulkan_buffer_create(
        context,
        sizeof(LocalUniform),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true,
        &out_shader->local_uniform_buffer
    ))
    {
        log_error("Failed to create local uniform buffer for object shader");
        return false;
    }

    out_shader->local_uniform_buffer_index = INVALID_ID + 1;
    return true;
}

void vulkan_obj_shader_destroy(VulkanContext* context, VulkanObjShader* shader)
{
    VkDevice device = context->device.logical_device;

    vulkan_buffer_destroy(context, &shader->global_uniform_buffer);
    vulkan_buffer_destroy(context, &shader->local_uniform_buffer);

    vulkan_pipeline_destroy(context, &shader->pipeline);

    vkDestroyDescriptorPool(device, shader->global_descriptor_pool, context->allocator);
    vkDestroyDescriptorPool(device, shader->local_descriptor_pool, context->allocator);
    vkDestroyDescriptorSetLayout(device, shader->global_descriptor_set_layout, context->allocator);
    vkDestroyDescriptorSetLayout(device, shader->local_descriptor_set_layout, context->allocator);

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

void vulkan_obj_shader_update_global_uniform(VulkanContext* context, VulkanObjShader* shader, f32 delta_time)
{
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].command_buffer;
    VkDescriptorSet descriptor_set = shader->global_descriptor_set[image_index];

    u32 range = sizeof(GlobalUniform);
    u64 offset = 0;

    vulkan_buffer_load_data(
        context,
        &shader->global_uniform_buffer,
        offset,
        range,
        0,
        &shader->global_uniform
    );

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = shader->global_uniform_buffer.buffer;
    buffer_info.offset = offset;
    buffer_info.range = range;

    VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_write.dstSet = descriptor_set;
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(context->device.logical_device, 1, &descriptor_write, 0, NULL);

    vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shader->pipeline.layout,
        0,
        1,
        &descriptor_set,
        0,
        NULL
    );
}

void vulkan_obj_shader_update_object(VulkanContext* context, VulkanObjShader* shader, GeometryRenderData render_data)
{
    u32 image_index = context->image_index;
    VkCommandBuffer command_buffer = context->graphics_command_buffers[image_index].command_buffer;

    vkCmdPushConstants(
        command_buffer,
        shader->pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(Mat4),
        &render_data.model
    );

    VulkanObjShaderState* state = &shader->object_states[render_data.object_id - 1];
    VkDescriptorSet descriptor_set = state->descriptor_sets[image_index];

    VkWriteDescriptorSet descriptor_writes[OBJECT_SHADER_DESCRIPTOR_COUNT] = {0};
    u32 descriptor_count = 0;
    u32 descriptor_index = 0;

    // Descriptor 0 - Uniform buffer
    u32 range = sizeof(LocalUniform);
    u64 offset = sizeof(LocalUniform) * (render_data.object_id - 1);
    LocalUniform local_uniform;

    static f32 color = 0.0f;
    color += context->frame_delta_time;
    f32 s = (math_sin(color) + 1.0f) / 2.0f;
    local_uniform.diffuse_color = (Vec4) {s, s, s, 1.0f};

    vulkan_buffer_load_data(
        context,
        &shader->local_uniform_buffer,
        offset,
        range,
        0,
        &local_uniform
    );

    if (state->descriptor_states[descriptor_index].generations[image_index] == INVALID_ID)
    {
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = shader->local_uniform_buffer.buffer;
        buffer_info.offset = offset;
        buffer_info.range = range;

        VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor_write.dstSet = descriptor_set;
        descriptor_write.dstBinding = descriptor_index;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        descriptor_writes[descriptor_count] = descriptor_write;
        descriptor_count++;

        state->descriptor_states[descriptor_index].generations[image_index] = 1;
    }

    descriptor_index++;

    const u32 sampler_count = 1;
    VkDescriptorImageInfo image_info[sampler_count];
    for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index)
    {
        Texture* t = render_data.textures[sampler_index];
        u32* generation = &state->descriptor_states[descriptor_index].generations[image_index];

        if (t && t->generation == INVALID_ID)
        {
            t = shader->default_diffuse;
            *generation = INVALID_ID;
        }

        if (t != NULL && (*generation != t->generation || *generation == INVALID_ID))
        {
            VulkanTexture* texture_data = (VulkanTexture*) t->data;
            image_info[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info[sampler_index].imageView = texture_data->image.view;
            image_info[sampler_index].sampler = texture_data->sampler;

            VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor_write.dstSet = descriptor_set;
            descriptor_write.dstBinding = descriptor_index;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pImageInfo = &image_info[sampler_index];

            descriptor_writes[descriptor_count] = descriptor_write;
            descriptor_count++;

            if (t->generation != INVALID_ID)
            {
                *generation = t->generation;
            }

            descriptor_index++;
        }
    }

    if (descriptor_count > 0)
    {
        vkUpdateDescriptorSets(context->device.logical_device, descriptor_count, descriptor_writes, 0, NULL);
    }

    vkCmdBindDescriptorSets(
        command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shader->pipeline.layout,
        1,
        1,
        &descriptor_set,
        0,
        NULL
    );
}

bool vulkan_obj_shader_acquire_resources(VulkanContext* context, VulkanObjShader* shader, u64* out_id)
{
    *out_id = shader->local_uniform_buffer_index;
    shader->local_uniform_buffer_index++;

    u32 obj_id = *out_id;
    VulkanObjShaderState* state = &shader->object_states[obj_id - 1];
    for (u32 i = 0; i < OBJECT_SHADER_DESCRIPTOR_COUNT; ++i)
    {
        for (u32 j = 0; j < 3; ++j)
        {
            state->descriptor_states[i].generations[j] = INVALID_ID;
        }
    }

    VkDescriptorSetLayout layouts[3] = 
    {
        shader->local_descriptor_set_layout,
        shader->local_descriptor_set_layout,
        shader->local_descriptor_set_layout
    };

    VkDescriptorSetAllocateInfo descriptor_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    descriptor_alloc_info.descriptorPool = shader->local_descriptor_pool;
    descriptor_alloc_info.descriptorSetCount = 3;
    descriptor_alloc_info.pSetLayouts = layouts;
    VkResult result = vkAllocateDescriptorSets(context->device.logical_device, &descriptor_alloc_info, state->descriptor_sets);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to allocate descriptor sets for object shader");
        return false;
    }

    return true;
}

void vulkan_obj_shader_release_resources(VulkanContext* context, VulkanObjShader* shader, u64 id)
{
    VulkanObjShaderState* state = &shader->object_states[id - 1];

    const u32 descriptor_count = 3;
    VK_ASSERT(vkFreeDescriptorSets(context->device.logical_device, shader->local_descriptor_pool, descriptor_count, state->descriptor_sets));

    for (u32 i = 0; i < OBJECT_SHADER_DESCRIPTOR_COUNT; ++i)
    {
        for (u32 j = 0; j < 3; ++j)
        {
            state->descriptor_states[i].generations[j] = INVALID_ID;
        }
    }
}