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
}