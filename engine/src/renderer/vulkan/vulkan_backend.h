#pragma once

#include "renderer/renderer_backend.h"

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name);
void vulkan_renderer_backend_shutdown(RendererBackend* backend);

bool vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 delta_time);
bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time);

void vulkan_renderer_update_global_uniform(Mat4 projection, Mat4 view, Vec3 view_positioni, Vec4 ambient_color, i32 mode);

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height);

void vulkan_renderer_create_texture(const u8* pixels, Texture* texture);
void vulkan_renderer_destroy_texture(Texture* texture);

bool vulkan_renderer_create_material(Material* material);
void vulkan_renderer_destroy_material(Material* material);

bool vulkan_renderer_create_geometry(Geometry* geometry, u32 vertex_count, const Vertex3d* vertices, u32 index_count, const u32* indices);
void vulkan_renderer_draw_geometry(GeometryRenderData data);
void vulkan_renderer_destroy_geometry(Geometry* geometry);