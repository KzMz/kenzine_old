#pragma once

#include "renderer/renderer_backend.h"

bool vulkan_renderer_backend_init(RendererBackend* backend, const char* app_name, struct Platform* platform);
void vulkan_renderer_backend_shutdown(RendererBackend* backend);

bool vulkan_renderer_backend_begin_frame(RendererBackend* backend, f64 delta_time);
bool vulkan_renderer_backend_end_frame(RendererBackend* backend, f64 delta_time);

void vulkan_renderer_backend_resize(RendererBackend* backend, i32 width, i32 height);