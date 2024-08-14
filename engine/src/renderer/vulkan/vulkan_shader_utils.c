#include "vulkan_shader_utils.h"
#include "vulkan_defines.h"
#include "lib/string.h"
#include "core/log.h"
#include "core/memory.h"
#include "platform/filesystem.h"

#define FILE_NAME_SIZE 512

bool create_shader_module(
    VulkanContext* context, 
    const char* shader_name, const char* stage_type, 
    VkShaderStageFlagBits stage_flag, u32 stage_index, VulkanShaderStage* stages)
{
    char file_name[FILE_NAME_SIZE];
    string_format(file_name, "assets/shaders/%s.%s.spv", shader_name, stage_type);

    memory_zero(&stages[stage_index].create_info, sizeof(VkShaderModuleCreateInfo));
    stages[stage_index].create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    FileHandle file_handle;
    if (!file_open(file_name, FILE_MODE_READ, true, &file_handle))
    {
        log_error("Failed to open shader file: %s", file_name);
        return false;
    }

    u64 size = 0;
    u8* shader_code = NULL;
    if (!file_read_all_bytes(&file_handle, &shader_code, &size))
    {
        log_error("Failed to read shader file: %s", file_name);
        file_close(&file_handle);
        return false;
    }

    stages[stage_index].create_info.codeSize = size;
    stages[stage_index].create_info.pCode = (u32*) shader_code;

    file_close(&file_handle);

    VK_ASSERT(vkCreateShaderModule(
        context->device.logical_device, 
        &stages[stage_index].create_info, context->allocator, &stages[stage_index].module));

    memory_zero(&stages[stage_index].stage_info, sizeof(VkPipelineShaderStageCreateInfo));  
    stages[stage_index].stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[stage_index].stage_info.stage = stage_flag;
    stages[stage_index].stage_info.module = stages[stage_index].module;
    stages[stage_index].stage_info.pName = "main";

    if (shader_code)
    {
        memory_free(shader_code, sizeof(u8) * size, MEMORY_TAG_STRING);
        shader_code = NULL;
    }

    return true;    
}