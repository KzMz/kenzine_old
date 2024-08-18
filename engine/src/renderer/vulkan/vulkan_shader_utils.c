#include "vulkan_shader_utils.h"
#include "vulkan_defines.h"
#include "lib/string.h"
#include "core/log.h"
#include "core/memory.h"
#include "systems/resource_system.h"

#define FILE_NAME_SIZE 512

bool create_shader_module(
    VulkanContext* context, 
    const char* shader_name, const char* stage_type, 
    VkShaderStageFlagBits stage_flag, u32 stage_index, VulkanShaderStage* stages)
{
    char file_name[FILE_NAME_SIZE];
    string_format(file_name, "shaders/%s.%s.spv", shader_name, stage_type);

    Resource shader_resource;
    if (!resource_system_load(file_name, RESOURCE_TYPE_BINARY, &shader_resource))
    {
        log_error("Failed to load shader file: %s", file_name);
        return false;
    }

    memory_zero(&stages[stage_index].create_info, sizeof(VkShaderModuleCreateInfo));
    stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    stages[stage_index].create_info.codeSize = shader_resource.size;
    stages[stage_index].create_info.pCode = (u32*) shader_resource.data;

    VK_ASSERT(vkCreateShaderModule(
        context->device.logical_device, 
        &stages[stage_index].create_info, context->allocator, &stages[stage_index].module));

    resource_system_unload(&shader_resource);

    memory_zero(&stages[stage_index].stage_info, sizeof(VkPipelineShaderStageCreateInfo));  
    stages[stage_index].stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[stage_index].stage_info.stage = stage_flag;
    stages[stage_index].stage_info.module = stages[stage_index].module;
    stages[stage_index].stage_info.pName = "main";

    return true;    
}