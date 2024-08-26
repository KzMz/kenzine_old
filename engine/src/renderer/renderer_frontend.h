#pragma once

#include "renderer_defines.h"

struct Platform;
struct Shader;
struct ShaderUniform;

bool renderer_init(void* state, const char* app_name);
void renderer_shutdown(void);

void renderer_resize(i32 width, i32 height);

bool renderer_draw_frame(RenderPacket* packet);

u64 renderer_get_state_size(void);

// TODO: remove it when not needed anymore
KENZINE_API void renderer_set_view(Mat4 view, Vec3 camera_position);

void renderer_create_texture(const u8* pixels, Texture* texture);
void renderer_destroy_texture(Texture* texture);

bool renderer_create_geometry
(
    Geometry* geometry, 
    u32 vertex_count, u32 vertex_size, const void* vertices, 
    u32 index_count, u32 index_size, const void* indices
);
void renderer_destroy_geometry(struct Geometry* geometry);

bool renderer_renderpass_id(const char* name, u8* out_renderpass_id);

bool renderer_shader_create(struct Shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_files, ShaderStage* stages);
void renderer_shader_destroy(struct Shader* shader);
bool renderer_shader_init(struct Shader* shader);

bool renderer_shader_use(struct Shader* shader);

bool renderer_shader_bind_globals(struct Shader* shader);
bool renderer_shader_bind_instance(struct Shader* shader, u64 instance_id);
bool renderer_shader_apply_globals(struct Shader* shader);
bool renderer_shader_apply_instance(struct Shader* shader);

bool renderer_shader_acquire_instance_resources(struct Shader* shader, u64* out_instance_id);
bool renderer_shader_release_instance_resources(struct Shader* shader, u64 instance_id);

bool renderer_shader_set_uniform(struct Shader* shader, struct ShaderUniform* uniform, const void* value);