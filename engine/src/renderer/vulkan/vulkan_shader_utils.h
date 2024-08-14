#pragma once

#include "vulkan_defines.h"

bool create_shader_module(VulkanContext* context, const char* shader_name, const char* stage_type, VkShaderStageFlagBits stage_flag, u32 stage_index, VulkanShaderStage* stages);