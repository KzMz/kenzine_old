#pragma once

#include "../vulkan_defines.h"
#include "../../renderer_defines.h"

bool vulkan_obj_shader_create(VulkanContext* context, Texture* default_diffuse, VulkanObjShader* out_shader);
void vulkan_obj_shader_destroy(VulkanContext* context, VulkanObjShader* shader);

void vulkan_obj_shader_use(VulkanContext* context, VulkanObjShader* shader);
void vulkan_obj_shader_update_global_uniform(VulkanContext* context, VulkanObjShader* shader, f32 delta_time);
void vulkan_obj_shader_update_object(VulkanContext* context, VulkanObjShader* shader, GeometryRenderData render_data);

bool vulkan_obj_shader_acquire_resources(VulkanContext* context, VulkanObjShader* shader, u64* out_id);
void vulkan_obj_shader_release_resources(VulkanContext* context, VulkanObjShader* shader, u64 id);