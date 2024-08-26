#pragma once

#include "renderer/renderer_backend.h"

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name);
void vulkan_renderer_backend_shutdown(RendererBackend* backend);

bool vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 delta_time);
bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time);

bool vulkan_renderer_begin_renderpass(RendererBackend* backend, u8 renderpass_id);
bool vulkan_renderer_end_renderpass(RendererBackend* backend, u8 renderpass_id);

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height);

void vulkan_renderer_create_texture(const u8* pixels, Texture* texture);
void vulkan_renderer_destroy_texture(Texture* texture);

bool vulkan_renderer_create_geometry(
    Geometry* geometry, 
    u32 vertex_count, u32 vertex_size, const void* vertices, 
    u32 index_count, u32 index_size, const void* indices
);
void vulkan_renderer_draw_geometry(GeometryRenderData data);
void vulkan_renderer_destroy_geometry(Geometry* geometry);

bool vulkan_renderer_create_shader(struct Shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_files, ShaderStage* stages);
void vulkan_renderer_destroy_shader(struct Shader* shader);

bool vulkan_renderer_shader_init(struct Shader* shader);
bool vulkan_renderer_shader_use(struct Shader* shader);
bool vulkan_renderer_shader_bind_globals(struct Shader* shader);
bool vulkan_renderer_shader_bind_instance(struct Shader* shader, u64 instance_id);
bool vulkan_renderer_shader_apply_globals(struct Shader* shader);
bool vulkan_renderer_shader_apply_instance(struct Shader* shader);
bool vulkan_renderer_shader_acquire_instance_resources(struct Shader* shader, u64* out_instance_id);
bool vulkan_renderer_shader_release_instance_resources(struct Shader* shader, u64 instance_id);
bool vulkan_renderer_set_uniform(struct Shader* shader, struct ShaderUniform* uniform, const void* value);