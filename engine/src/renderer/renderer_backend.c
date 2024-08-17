#include "renderer_backend.h"

#include "vulkan/vulkan_backend.h"

bool renderer_backend_create(RendererBackendType type, RendererBackend* out_backend)
{
    if (type == RENDERER_BACKEND_TYPE_VULKAN)
    {
        out_backend->init = vulkan_renderer_backend_init;
        out_backend->shutdown = vulkan_renderer_backend_shutdown;
        out_backend->resize = vulkan_renderer_backend_resize;
        out_backend->begin_frame = vulkan_renderer_backend_begin_frame;
        out_backend->end_frame = vulkan_renderer_backend_end_frame;
        out_backend->update_global_uniform = vulkan_renderer_update_global_uniform;
        out_backend->update_model = vulkan_renderer_update_model;
        out_backend->create_texture = vulkan_renderer_create_texture;
        out_backend->destroy_texture = vulkan_renderer_destroy_texture;
        out_backend->create_material = vulkan_renderer_create_material;
        out_backend->destroy_material = vulkan_renderer_destroy_material;

        return true;
    }

    return false;
}

void renderer_backend_destroy(RendererBackend* backend)
{
    backend->init = 0;
    backend->shutdown = 0;
    backend->resize = 0;
    backend->begin_frame = 0;
    backend->end_frame = 0;
    backend->update_global_uniform = 0;
    backend->update_model = 0;
    backend->create_texture = 0;
    backend->destroy_texture = 0;
    backend->create_material = 0;
    backend->destroy_material = 0;
}