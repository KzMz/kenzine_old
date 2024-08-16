#pragma once

#include "defines.h"
#include "lib/math/math_defines.h"
#include "resources/resource_defines.h"

typedef enum RendererBackendType
{
    RENDERER_BACKEND_TYPE_VULKAN = 0,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
    RENDERER_BACKEND_TYPE_WEBGPU
} RendererBackendType;

typedef struct GlobalUniform
{
    Mat4 projection;
    Mat4 view;
    Mat4 reserved0;
    Mat4 reserved1;
} GlobalUniform;

typedef struct LocalUniform
{
    Vec4 diffuse_color;
    Vec4 reserved0;
    Vec4 reserved1;
    Vec4 reserved2;
} LocalUniform;

#define MAX_TEXTURES 16

typedef struct GeometryRenderData
{
    u64 object_id;
    Mat4 model;
    Texture* textures[MAX_TEXTURES];
} GeometryRenderData;

struct RendererBackend;
struct Platform;

typedef bool (*RendererBackendInit)(struct RendererBackend* backend, const char* app_name);
typedef void (*RendererBackendShutdown)(struct RendererBackend* backend);
typedef void (*RendererBackendResize)(struct RendererBackend* backend, i32 width, i32 height);
typedef bool (*RendererBackendBeginFrame)(struct RendererBackend* backend, f64 delta_time);
typedef bool (*RendererBackendEndFrame)(struct RendererBackend* backend, f64 delta_time);
typedef void (*RendererBackendUpdateGlobalUniform)(Mat4 proj, Mat4 view, Vec3 view_position, Vec4 ambient_color, i32 mode);
typedef void (*RendererBackendUpdateModel)(GeometryRenderData data);
typedef void (*RendererBackendCreateTexture)(const char* name, i32 width, i32 height, u8 channel_count, const u8* pixels, bool has_transparency, Texture* out_texture);
typedef void (*RendererBackendDestroyTexture)(Texture* texture);

typedef struct RendererBackend 
{
    u64 frame_number;

    RendererBackendInit init;
    RendererBackendShutdown shutdown;

    RendererBackendResize resize;

    RendererBackendBeginFrame begin_frame;
    RendererBackendEndFrame end_frame;

    RendererBackendUpdateGlobalUniform update_global_uniform;
    RendererBackendUpdateModel update_model;

    RendererBackendCreateTexture create_texture;
    RendererBackendDestroyTexture destroy_texture;
} RendererBackend;

typedef struct RenderPacket 
{
    f64 delta_time;
} RenderPacket;