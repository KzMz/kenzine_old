#pragma once

#include "../vulkan_defines.h"
#include "../../renderer_defines.h"

bool vulkan_material_shader_create(VulkanContext* context, VulkanMaterialShader* out_shader);
void vulkan_material_shader_destroy(VulkanContext* context, VulkanMaterialShader* shader);

void vulkan_material_shader_use(VulkanContext* context, VulkanMaterialShader* shader);
void vulkan_material_shader_update_global_uniform(VulkanContext* context, VulkanMaterialShader* shader, f32 delta_time);

void vulkan_material_shader_set_model(VulkanContext* context, VulkanMaterialShader* shader, Mat4 model);
void vulkan_material_shader_apply_material(VulkanContext* context, VulkanMaterialShader* shader, Material* material);

bool vulkan_material_shader_acquire_resources(VulkanContext* context, VulkanMaterialShader* shader, Material* material);
void vulkan_material_shader_release_resources(VulkanContext* context, VulkanMaterialShader* shader, Material* material);