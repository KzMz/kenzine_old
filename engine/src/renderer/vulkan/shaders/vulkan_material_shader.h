#pragma once

#include "../vulkan_defines.h"
#include "../../renderer_defines.h"

bool vulkan_material_shader_create(VulkanContext* context, Texture* default_diffuse, VulkanMaterialShader* out_shader);
void vulkan_material_shader_destroy(VulkanContext* context, VulkanMaterialShader* shader);

void vulkan_material_shader_use(VulkanContext* context, VulkanMaterialShader* shader);
void vulkan_material_shader_update_global_uniform(VulkanContext* context, VulkanMaterialShader* shader, f32 delta_time);
void vulkan_material_shader_update_object(VulkanContext* context, VulkanMaterialShader* shader, GeometryRenderData render_data);

bool vulkan_material_shader_acquire_resources(VulkanContext* context, VulkanMaterialShader* shader, u64* out_id);
void vulkan_material_shader_release_resources(VulkanContext* context, VulkanMaterialShader* shader, u64 id);