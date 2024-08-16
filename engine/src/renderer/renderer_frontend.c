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

// TODO: temporary
#include "lib/string.h"
#include "core/event.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
// TODO: end temporary

typedef struct RendererState
{
    RendererBackend* backend;
    Mat4 projection;
    Mat4 view;
    f32 near_clip;
    f32 far_clip;

    Texture default_texture;

    // TODO: temporary
    Texture test_diffuse;
    // TODO: end temporary
} RendererState;

static RendererState* renderer_state = 0;

void create_texture(Texture* t)
{
    memory_zero(t, sizeof(Texture));
    t->generation = INVALID_ID;
}

bool load_texture(const char* texture_name, Texture* out_texture)
{
    char* format_str = "../assets/textures/%s.%s";
    const i32 required_channel_count = 4;
    stbi_set_flip_vertically_on_load(true);
    
    const i32 max_path_length = 512;
    char path[max_path_length];
    string_format(path, format_str, texture_name, "png");

    Texture tmp;
    u8* data = stbi_load(
        path,
        (i32*) &tmp.width,
        (i32*) &tmp.height,
        (i32*) &tmp.channel_count,
        required_channel_count
    );

    tmp.channel_count = required_channel_count;

    if (data == NULL)
    {
        log_warning("Failed to load texture: %s. Reason: %s", path, stbi_failure_reason());
        return false;
    }

    u32 generation = out_texture->generation;
    out_texture->generation = INVALID_ID;

    u64 total_size = tmp.width * tmp.height * tmp.channel_count;
    bool has_transparency = false;
    for (i64 i = 0; i < total_size; i += tmp.channel_count)
    {
        u8 alpha = data[i + 3];
        if (alpha < 255)
        {
            has_transparency = true;
            break;
        }
    }

    if (stbi_failure_reason())
    {
        log_warning("Failed to load texture: %s. Reason: %s", path, stbi_failure_reason());
        return false;
    }

    renderer_create_texture(
        texture_name,
        tmp.width, tmp.height,  
        tmp.channel_count, 
        data,
        has_transparency,
        true,
        &tmp
    );

    Texture old = *out_texture;
    *out_texture = tmp;

    renderer_destroy_texture(&old);

    if (generation == INVALID_ID)
    {
        out_texture->generation = 1;
    }
    else
    {
        out_texture->generation = generation + 1;
    }

    stbi_image_free(data);
    return true;
}

// TODO: temporary
bool event_on_debug(u16 code, void* sender, void* listener, EventContext context)
{
    const char* names[4] = {
        "cobblestone",
        "dadobax",
        "paving",
        "paving2"
    };
    static i8 choice = 2;
    choice = (choice + 1) % 4;

    load_texture(names[choice], &renderer_state->test_diffuse);
    return true;
}
// TODO: end temporary

bool renderer_init(void* state, const char* app_name)
{
    renderer_state = (RendererState*) state;
    renderer_state->backend = (RendererBackend*) memory_alloc(sizeof(RendererBackend), MEMORY_TAG_RENDERER);

    // TODO: temporary
    event_subscribe(EVENT_CODE_DEBUG0, renderer_state, event_on_debug);
    // TODO: end temporary

    renderer_state->backend->default_diffuse = &renderer_state->default_texture;

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

    // NOTE: Create default texture
    const u32 texture_size = 256;
    const u32 bpp = 4;
    const u32 pixels_count = texture_size * texture_size * bpp;
    u8 pixels[pixels_count];
    for (u32 i = 0; i < pixels_count; i++)
    {
        pixels[i] = 255;
    }

    for (u64 row = 0; row < texture_size; row++)
    {
        for (u64 col = 0; col < texture_size; col++)
        {
            u64 index = (row * texture_size + col) * bpp;
            if (row % 2 == col % 2)
            {
                pixels[index + 0] = 0;
                pixels[index + 1] = 0;
            }
        }
    }

    renderer_create_texture(
        "default",
        texture_size, texture_size,
        bpp,
        pixels,
        false,
        false,
        &renderer_state->default_texture
    );

    renderer_state->default_texture.generation = INVALID_ID;

    create_texture(&renderer_state->test_diffuse);
    return true;
}

void renderer_shutdown(void)
{
    if (!renderer_state)
    {
        log_warning("Renderer is not initialized. Nothing to shutdown.");
        return;
    }

    // TODO: temporary
    event_unsubscribe(EVENT_CODE_DEBUG0, renderer_state, event_on_debug);
    // TODO: end temporary

    renderer_destroy_texture(&renderer_state->default_texture);
    renderer_destroy_texture(&renderer_state->test_diffuse);

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
        GeometryRenderData data = { 0 };
        data.object_id = 1;
        data.model = model;
        data.textures[0] = &renderer_state->test_diffuse;
        renderer_state->backend->update_model(data);

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

void renderer_create_texture(
    const char* name, 
    i32 width, i32 height, 
    u8 channel_count, const u8* pixels, 
    bool has_transparency, bool auto_release, 
    Texture* out_texture)
{
    renderer_state->backend->create_texture(name, width, height, channel_count, pixels, has_transparency, auto_release, out_texture);
}

void renderer_destroy_texture(Texture* texture)
{
    renderer_state->backend->destroy_texture(texture);
}