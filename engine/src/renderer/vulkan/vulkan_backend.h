#pragma once

#include "renderer/renderer_backend.h"

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name);
void vulkan_renderer_backend_shutdown(RendererBackend* backend);

bool vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 delta_time);
bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time);

bool vulkan_renderer_begin_renderpass(RendererBackend* backend, u8 renderpass_id);
bool vulkan_renderer_end_renderpass(RendererBackend* backend, u8 renderpass_id);

void vulkan_renderer_update_global_world_uniform(Mat4 projection, Mat4 view, Vec3 view_position, Vec4 ambient_color, i32 mode);
void vulkan_renderer_update_global_ui_uniform(Mat4 projection, Mat4 view, i32 mode);

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height);

void vulkan_renderer_create_texture(const u8* pixels, Texture* texture);
void vulkan_renderer_destroy_texture(Texture* texture);

bool vulkan_renderer_create_material(Material* material);
void vulkan_renderer_destroy_material(Material* material);

bool vulkan_renderer_create_geometry(
    Geometry* geometry, 
    u32 vertex_count, u32 vertex_size, const void* vertices, 
    u32 index_count, u32 index_size, const void* indices
);
void vulkan_renderer_draw_geometry(GeometryRenderData data);
void vulkan_renderer_destroy_geometry(Geometry* geometry);