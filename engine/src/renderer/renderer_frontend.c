#include "renderer_frontend.h"
#include "renderer_backend.h"
#include "core/log.h"
#include "core/memory.h"
#include "lib/math/math_defines.h"
#include "lib/math/vec3.h"
#include "lib/math/vec4.h"
#include "lib/math/mat4.h"
#include "lib/math/quat.h"

typedef struct RendererState
{
    RendererBackend* backend;
    Mat4 projection;
    Mat4 view;
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

bool renderer_begin_frame(f64 delta_time)
{
    return renderer_state->backend->begin_frame(renderer_state->backend, delta_time);
}

bool renderer_end_frame(f64 delta_time)
{
    bool result = renderer_state->backend->end_frame(renderer_state->backend, delta_time);
    renderer_state->backend->frame_number++;
    return result;
}

bool renderer_draw_frame(RenderPacket* packet)
{
    if (renderer_begin_frame(packet->delta_time))
    {
        renderer_state->backend->update_global_uniform(renderer_state->projection, renderer_state->view, vec3_zero(), vec4_one(), 0);

        static f32 angle = 0.01f;
        angle += 0.001f;

        Quat rotation = quat_from_axis_angle(vec3_forward(), angle, false);
        Mat4 model = quat_to_rot_mat4(rotation, vec3_zero());
        renderer_state->backend->update_model(model);

        bool result = renderer_end_frame(packet->delta_time);
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