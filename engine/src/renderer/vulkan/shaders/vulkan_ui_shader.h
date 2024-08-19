#pragma once

#include "renderer/vulkan/vulkan_defines.h"
#include "lib/math/math_defines.h"
#include "renderer/renderer_defines.h"

bool vulkan_ui_shader_create(VulkanContext* context, VulkanUIShader* out_shader);
void vulkan_ui_shader_destroy(VulkanContext* context, VulkanUIShader* shader);

void vulkan_ui_shader_use(VulkanContext* context, VulkanUIShader* shader);
void vulkan_ui_shader_update_global_uniform(VulkanContext* context, VulkanUIShader* shader, f32 delta_time);

void vulkan_ui_shader_set_model(VulkanContext* context, VulkanUIShader* shader, Mat4 model);
void vulkan_ui_shader_apply_material(VulkanContext* context, VulkanUIShader* shader, Material* material);

bool vulkan_ui_shader_acquire_resources(VulkanContext* context, VulkanUIShader* shader, Material* material);
void vulkan_ui_shader_release_resources(VulkanContext* context, VulkanUIShader* shader, Material* material);