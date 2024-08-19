#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "core/log.h"
#include "core/memory.h"
#include "lib/math/math_defines.h"
#include "lib/math/vec3.h"
#include "lib/math/vec4.h"
#include "lib/math/mat4.h"
#include "lib/math/quat.h"
#include "resources/resource_defines.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"

// TODO: temporary
#include "lib/string.h"
#include "core/event.h"
// TODO: end temporary

typedef struct RendererState
{
    RendererBackend* backend;
    Mat4 projection;
    Mat4 view;
    Mat4 ui_projection;
    Mat4 ui_view;
    f32 near_clip;
    f32 far_clip;
} RendererState;

static RendererState* renderer_state = 0;

bool renderer_init(void* state, const char* app_name)
{
    renderer_state = (RendererState*) state;
    renderer_state->backend = (RendererBackend*) memory_alloc(sizeof(RendererBackend), MEMORY_TAG_RENDERER);

    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, renderer_state->backend);
    renderer_state->backend->frame_number = 0;

    if (!renderer_state->backend->init(renderer_state->backend, app_name))
    {
        log_fatal("Failed to initialize renderer backend. Shutting down.");
        return false;
    }

    renderer_state->near_clip = 0.1f;
    renderer_state->far_clip = 1000.0f;
    renderer_state->projection = mat4_proj_perspective(deg_to_rad(45.0f), 1280 / 720.0f, renderer_state->near_clip, renderer_state->far_clip);
    
    Mat4 view = mat4_translation((Vec3) { 0, 0, 30 });
    renderer_state->view = mat4_inverse(view);

    renderer_state->ui_projection = mat4_proj_orthographic(0, 1280.0f, 720.0f, 0, -100.0f, 100.0f);
    renderer_state->ui_view = mat4_inverse(mat4_identity());

    return true;
}

void renderer_shutdown(void)
{
    if (!renderer_state)
    {
        log_warning("Renderer is not initialized. Nothing to shutdown.");
        return;
    }
    
    renderer_state->backend->shutdown(renderer_state->backend);
    renderer_backend_destroy(renderer_state->backend);
    
    memory_free(renderer_state->backend, sizeof(RendererBackend), MEMORY_TAG_RENDERER);
}

bool renderer_draw_frame(RenderPacket* packet)
{
    if (renderer_state->backend->begin_frame(renderer_state->backend, packet->delta_time))
    {
        if (!renderer_state->backend->begin_renderpass(renderer_state->backend, BUILTIN_RENDERPASS_WORLD))
        {
            log_error("Failed to begin world renderpass. Shutting down...");
            return false;
        }

        renderer_state->backend->update_global_world_uniform(renderer_state->projection, renderer_state->view, vec3_zero(), vec4_one(), 0);

        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; i++)
        {
            renderer_state->backend->draw_geometry(packet->geometries[i]);
        }

        if (!renderer_state->backend->end_renderpass(renderer_state->backend, BUILTIN_RENDERPASS_WORLD))
        {
            log_error("Failed to end world renderpass. Shutting down...");
            return false;
        }

        if (!renderer_state->backend->begin_renderpass(renderer_state->backend, BUILTIN_RENDERPASS_UI))
        {
            log_error("Failed to begin ui renderpass. Shutting down...");
            return false;
        }

        renderer_state->backend->update_global_ui_uniform(renderer_state->ui_projection, renderer_state->ui_view, 0);

        count = packet->ui_geometry_count;
        for (u32 i = 0; i < count; i++)
        {
            renderer_state->backend->draw_geometry(packet->ui_geometries[i]);
        }

        if (!renderer_state->backend->end_renderpass(renderer_state->backend, BUILTIN_RENDERPASS_UI))
        {
            log_error("Failed to end ui renderpass. Shutting down...");
            return false;
        }

        bool result = renderer_state->backend->end_frame(renderer_state->backend, packet->delta_time);
        if (!result) 
        {
            log_error("Failed to end draw frame. Shutting down...");
            return false;
        }
    }

    return true;
}

void renderer_resize(i32 width, i32 height)
{
    if (!renderer_state)
    {
        log_warning("Renderer is not initialized. Cannot resize.");
        return;
    }

    renderer_state->projection = mat4_proj_perspective(deg_to_rad(45.0f), width / (f32) height, renderer_state->near_clip, renderer_state->far_clip);
    renderer_state->ui_projection = mat4_proj_orthographic(0, width, height, 0, -100.0f, 100.0f);

    if (renderer_state->backend)
    {
        renderer_state->backend->resize(renderer_state->backend, width, height);
    }
    else 
    {
        log_warning("Renderer backend is not initialized. Cannot resize.");
    }
}

u64 renderer_get_state_size(void)
{
    return sizeof(RendererState);
}

void renderer_set_view(Mat4 view)
{
    renderer_state->view = view;
}

void renderer_create_texture(const u8* pixels, Texture* texture)
{
    renderer_state->backend->create_texture(pixels, texture);
}

void renderer_destroy_texture(Texture* texture)
{
    renderer_state->backend->destroy_texture(texture);
}

bool renderer_create_material(struct Material* material)
{
    return renderer_state->backend->create_material(material);
}

void renderer_destroy_material(struct Material* material)
{
    renderer_state->backend->destroy_material(material);
}

bool renderer_create_geometry(struct Geometry* geometry, u32 vertex_count, const Vertex3d* vertices, u32 index_count, const u32* indices)
{
    return renderer_state->backend->create_geometry(geometry, vertex_count, vertices, index_count, indices);
}

void renderer_destroy_geometry(struct Geometry* geometry)
{
    renderer_state->backend->destroy_geometry(geometry);
}