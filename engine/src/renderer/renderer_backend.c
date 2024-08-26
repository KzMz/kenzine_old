#include "renderer_backend.h"

#include "vulkan/vulkan_backend.h"
#include "core/memory.h"

bool renderer_backend_create(RendererBackendType type, RendererBackend* out_backend)
{
    if (type == RENDERER_BACKEND_TYPE_VULKAN)
    {
        out_backend->init = vulkan_renderer_backend_init;
        out_backend->shutdown = vulkan_renderer_backend_shutdown;
        out_backend->resize = vulkan_renderer_backend_resize;
        out_backend->begin_frame = vulkan_renderer_backend_begin_frame;
        out_backend->end_frame = vulkan_renderer_backend_end_frame;
        out_backend->create_geometry = vulkan_renderer_create_geometry;
        out_backend->draw_geometry = vulkan_renderer_draw_geometry;
        out_backend->destroy_geometry = vulkan_renderer_destroy_geometry;
        out_backend->create_texture = vulkan_renderer_create_texture;
        out_backend->destroy_texture = vulkan_renderer_destroy_texture;
        out_backend->begin_renderpass = vulkan_renderer_begin_renderpass;
        out_backend->end_renderpass = vulkan_renderer_end_renderpass;
        out_backend->create_shader = vulkan_renderer_create_shader;
        out_backend->destroy_shader = vulkan_renderer_destroy_shader;
        out_backend->init_shader = vulkan_renderer_shader_init;
        out_backend->use_shader = vulkan_renderer_shader_use;
        out_backend->bind_shader_globals = vulkan_renderer_shader_bind_globals;
        out_backend->bind_shader_instance = vulkan_renderer_shader_bind_instance;
        out_backend->apply_shader_instance = vulkan_renderer_shader_apply_instance;
        out_backend->apply_shader_globals = vulkan_renderer_shader_apply_globals;
        out_backend->acquire_shader_instance_resources = vulkan_renderer_shader_acquire_instance_resources;
        out_backend->release_shader_instance_resources = vulkan_renderer_shader_release_instance_resources;
        out_backend->set_shader_uniform = vulkan_renderer_set_uniform;

        return true;
    }

    return false;
}

void renderer_backend_destroy(RendererBackend* backend)
{
    memory_zero(backend, sizeof(RendererBackend));
}