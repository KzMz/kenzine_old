#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "core/log.h"
#include "core/memory.h"

static RendererBackend* backend = 0;

bool renderer_init(const char* app_name, struct Platform* platform)
{
    backend = (RendererBackend*) memory_alloc(sizeof(RendererBackend), MEMORY_TAG_RENDERER);
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, platform, backend);
    backend->frame_number = 0;

    if (!backend->init(backend, app_name, platform))
    {
        log_fatal("Failed to initialize renderer backend. Shutting down.");
        return false;
    }

    return true;
}

void renderer_shutdown(void)
{
    backend->shutdown(backend);
    renderer_backend_destroy(backend);
    
    memory_free(backend, sizeof(RendererBackend), MEMORY_TAG_RENDERER);
}

bool renderer_begin_frame(f64 delta_time)
{
    return backend->begin_frame(backend, delta_time);
}

bool renderer_end_frame(f64 delta_time)
{
    bool result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

bool renderer_draw_frame(RenderPacket* packet)
{
    if (renderer_begin_frame(packet->delta_time))
    {
        bool result = renderer_end_frame(packet->delta_time);
        if (!result) 
        {
            log_error("Failed to end draw frame. Shutting down...");
            return false;
        }
    }

    return true;
}