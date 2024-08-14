#pragma once

#include "defines.h"
#include "lib/math/math_defines.h"

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

struct RendererBackend;
struct Platform;

typedef bool (*RendererBackendInit)(struct RendererBackend* backend, const char* app_name);
typedef void (*RendererBackendShutdown)(struct RendererBackend* backend);
typedef void (*RendererBackendResize)(struct RendererBackend* backend, i32 width, i32 height);
typedef bool (*RendererBackendBeginFrame)(struct RendererBackend* backend, f64 delta_time);
typedef bool (*RendererBackendEndFrame)(struct RendererBackend* backend, f64 delta_time);
typedef void (*RendererBackendUpdateGlobalUniform)(Mat4 proj, Mat4 view, Vec3 view_position, Vec4 ambient_color, i32 mode);

typedef struct RendererBackend 
{
    u64 frame_number;

    RendererBackendInit init;
    RendererBackendShutdown shutdown;

    RendererBackendResize resize;

    RendererBackendBeginFrame begin_frame;
    RendererBackendEndFrame end_frame;

    RendererBackendUpdateGlobalUniform update_global_uniform;
} RendererBackend;

typedef struct RenderPacket 
{
    f64 delta_time;
} RenderPacket;