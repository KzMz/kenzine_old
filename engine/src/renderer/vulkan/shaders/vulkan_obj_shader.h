#pragma once

#include "../vulkan_defines.h"
#include "../../renderer_defines.h"

bool vulkan_obj_shader_create(VulkanContext* context, VulkanObjShader* out_shader);
void vulkan_obj_shader_destroy(VulkanContext* context, VulkanObjShader* shader);
void vulkan_obj_shader_use(VulkanContext* context, VulkanObjShader* shader);
void vulkan_obj_shader_update_global_uniform(VulkanContext* context, VulkanObjShader* shader);