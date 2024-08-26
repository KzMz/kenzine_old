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
#include "systems/resource_system.h"
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"

// TODO: temporary
#include "lib/string.h"
#include "core/event.h"
// TODO: end temporary

typedef struct RendererState
{
    RendererBackend backend;
    Mat4 projection;
    Mat4 view;
    Vec4 ambient_color;
    Mat4 ui_projection;
    Mat4 ui_view;
    f32 near_clip;
    f32 far_clip;
    u64 material_shader_id;
    u64 ui_shader_id;
} RendererState;

static RendererState* renderer_state = 0;

#define CRITICAL(op, msg) if (!(op)) { log_error(msg); return false; }

bool renderer_init(void* state, const char* app_name)
{
    renderer_state = (RendererState*) state;

    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &renderer_state->backend);
    renderer_state->backend.frame_number = 0;

    CRITICAL(renderer_state->backend.init(&renderer_state->backend, app_name), "Failed to initialize renderer backend. Shutting down.");

    Resource config_resource;
    ShaderConfig* config = NULL;

    CRITICAL(resource_system_load(BUILTIN_SHADER_NAME_MATERIAL, RESOURCE_TYPE_SHADER, &config_resource), "Failed to load material shader config.");
    config = (ShaderConfig*) config_resource.data;
    CRITICAL(shader_system_create(config), "Failed to create material shader.");
    resource_system_unload(&config_resource);
    renderer_state->material_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_MATERIAL);

    CRITICAL(resource_system_load(BUILTIN_SHADER_NAME_UI, RESOURCE_TYPE_SHADER, &config_resource), "Failed to load ui shader config.");
    config = (ShaderConfig*) config_resource.data;
    CRITICAL(shader_system_create(config), "Failed to create ui shader.");
    resource_system_unload(&config_resource);
    renderer_state->ui_shader_id = shader_system_get_id(BUILTIN_SHADER_NAME_UI);

    renderer_state->near_clip = 0.1f;
    renderer_state->far_clip = 1000.0f;
    renderer_state->projection = mat4_proj_perspective(deg_to_rad(45.0f), 1280 / 720.0f, renderer_state->near_clip, renderer_state->far_clip);
    
    Mat4 view = mat4_translation((Vec3) { 0, 0, 30 });
    renderer_state->view = mat4_inverse(view);

    renderer_state->ambient_color = (Vec4) { 0.25f, 0.25f, 0.25f, 1.0f };

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
    
    renderer_state->backend.shutdown(&renderer_state->backend);
    renderer_backend_destroy(&renderer_state->backend);
}

bool renderer_draw_frame(RenderPacket* packet)
{
    if (renderer_state->backend.begin_frame(&renderer_state->backend, packet->delta_time))
    {
        if (!renderer_state->backend.begin_renderpass(&renderer_state->backend, BUILTIN_RENDERPASS_WORLD))
        {
            log_error("Failed to begin world renderpass. Shutting down...");
            return false;
        }

        if (!shader_system_use_by_id(renderer_state->material_shader_id))
        {
            log_error("Failed to use material shader. Render frame failed.");
            return false;
        }

        if (!material_system_apply_global(renderer_state->material_shader_id, &renderer_state->projection, &renderer_state->view, &renderer_state->ambient_color))
        {
            log_error("Failed to apply global material shader uniforms. Render frame failed.");
            return false;
        }

        u32 count = packet->geometry_count;
        for (u32 i = 0; i < count; i++)
        {
            Material* mat = NULL;
            if (packet->geometries[i].geometry->material != NULL)
            {
                mat = packet->geometries[i].geometry->material;
            }
            else 
            {
                mat = material_system_get_default();
            }

            if (!material_system_apply_instance(mat))
            {
                log_warning("Failed to apply material instance %s. Skipping geometry...", mat->name);
                return false;
            }

            material_system_apply_local(mat, &packet->geometries[i].model);

            renderer_state->backend.draw_geometry(packet->geometries[i]);
        }

        if (!renderer_state->backend.end_renderpass(&renderer_state->backend, BUILTIN_RENDERPASS_WORLD))
        {
            log_error("Failed to end world renderpass. Shutting down...");
            return false;
        }

        if (!renderer_state->backend.begin_renderpass(&renderer_state->backend, BUILTIN_RENDERPASS_UI))
        {
            log_error("Failed to begin ui renderpass. Shutting down...");
            return false;
        }

        if (!shader_system_use_by_id(renderer_state->ui_shader_id))
        {
            log_error("Failed to use ui shader. Render frame failed.");
            return false;
        }

        if (!material_system_apply_global(renderer_state->ui_shader_id, &renderer_state->ui_projection, &renderer_state->ui_view, NULL))
        {
            log_error("Failed to apply global ui shader uniforms. Render frame failed.");
            return false;
        }

        count = packet->ui_geometry_count;
        for (u32 i = 0; i < count; i++)
        {
            Material* mat = NULL;
            if (packet->ui_geometries[i].geometry->material != NULL)
            {
                mat = packet->ui_geometries[i].geometry->material;
            }
            else 
            {
                mat = material_system_get_default();
            }

            if (!material_system_apply_instance(mat))
            {
                log_warning("Failed to apply material instance %s. Skipping geometry...", mat->name);
                return false;
            }

            material_system_apply_local(mat, &packet->ui_geometries[i].model);

            renderer_state->backend.draw_geometry(packet->ui_geometries[i]);
        }

        if (!renderer_state->backend.end_renderpass(&renderer_state->backend, BUILTIN_RENDERPASS_UI))
        {
            log_error("Failed to end ui renderpass. Shutting down...");
            return false;
        }

        bool result = renderer_state->backend.end_frame(&renderer_state->backend, packet->delta_time);
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

    renderer_state->backend.resize(&renderer_state->backend, width, height);
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
    renderer_state->backend.create_texture(pixels, texture);
}

void renderer_destroy_texture(Texture* texture)
{
    renderer_state->backend.destroy_texture(texture);
}

bool renderer_create_geometry
(    
    Geometry* geometry, 
    u32 vertex_count, u32 vertex_size, const void* vertices, 
    u32 index_count, u32 index_size, const void* indices
)
{
    return renderer_state->backend.create_geometry(geometry, vertex_count, vertex_size, vertices, index_count, index_size, indices);
}

void renderer_destroy_geometry(struct Geometry* geometry)
{
    renderer_state->backend.destroy_geometry(geometry);
}

bool renderer_renderpass_id(const char* name, u8* out_renderpass_id)
{
    if (string_equals_nocase("Renderpass.Builtin.World", name))
    {
        *out_renderpass_id = BUILTIN_RENDERPASS_WORLD;
        return true;
    }
    else if (string_equals_nocase("Renderpass.Builtin.UI", name))
    {
        *out_renderpass_id = BUILTIN_RENDERPASS_UI;
        return true;
    }

    log_error("Unknown renderpass name: %s", name);
    *out_renderpass_id = INVALID_ID_U8;
    return false;
}

bool renderer_shader_create(struct Shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_files, ShaderStage* stages)
{
    return renderer_state->backend.create_shader(shader, renderpass_id, stage_count, stage_files, stages);
}

void renderer_shader_destroy(struct Shader* shader)
{
    renderer_state->backend.destroy_shader(shader);
}

bool renderer_shader_init(struct Shader* shader)
{
    return renderer_state->backend.init_shader(shader);
}

bool renderer_shader_use(struct Shader* shader)
{
    return renderer_state->backend.use_shader(shader);
}

bool renderer_shader_bind_globals(struct Shader* shader)
{
    return renderer_state->backend.bind_shader_globals(shader);
}

bool renderer_shader_bind_instance(struct Shader* shader, u64 instance_id)
{
    return renderer_state->backend.bind_shader_instance(shader, instance_id);
}

bool renderer_shader_apply_globals(struct Shader* shader)
{
    return renderer_state->backend.apply_shader_globals(shader);
}

bool renderer_shader_apply_instance(struct Shader* shader)
{
    return renderer_state->backend.apply_shader_instance(shader);
}

bool renderer_shader_acquire_instance_resources(struct Shader* shader, u64* out_instance_id)
{
    return renderer_state->backend.acquire_shader_instance_resources(shader, out_instance_id);
}

bool renderer_shader_release_instance_resources(struct Shader* shader, u64 instance_id)
{
    return renderer_state->backend.release_shader_instance_resources(shader, instance_id);
}

bool renderer_shader_set_uniform(struct Shader* shader, struct ShaderUniform* uniform, const void* value)
{
    return renderer_state->backend.set_shader_uniform(shader, uniform, value);
}