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

typedef struct MaterialUniform
{
    Vec4 diffuse_color;
    Vec4 reserved0;
    Vec4 reserved1;
    Vec4 reserved2;
} MaterialUniform;

typedef struct GeometryRenderData
{
    Mat4 model;
    Geometry* geometry;
} GeometryRenderData;

struct RendererBackend;
struct Platform;

typedef bool (*RendererBackendInit)(struct RendererBackend* backend, const char* app_name);
typedef void (*RendererBackendShutdown)(struct RendererBackend* backend);
typedef void (*RendererBackendResize)(struct RendererBackend* backend, i32 width, i32 height);
typedef bool (*RendererBackendBeginFrame)(struct RendererBackend* backend, f64 delta_time);
typedef bool (*RendererBackendEndFrame)(struct RendererBackend* backend, f64 delta_time);
typedef void (*RendererBackendUpdateGlobalUniform)(Mat4 proj, Mat4 view, Vec3 view_position, Vec4 ambient_color, i32 mode);
typedef void (*RendererBackendCreateTexture)(const u8* pixels, Texture* texture);
typedef void (*RendererBackendDestroyTexture)(Texture* texture);
typedef bool (*RendererBackendCreateMaterial)(Material* material);
typedef void (*RendererBackendDestroyMaterial)(Material* material);
typedef bool (*RendererBackendCreateGeometry)(Geometry* geometry, u32 vertex_count, const Vertex3d* vertices, u32 index_count, const u32* indices);
typedef void (*RendererBackendDrawGeometry)(GeometryRenderData data);
typedef void (*RendererBackendDestroyGeometry)(Geometry* geometry);

typedef struct RendererBackend 
{
    u64 frame_number;

    RendererBackendInit init;
    RendererBackendShutdown shutdown;

    RendererBackendResize resize;

    RendererBackendBeginFrame begin_frame;
    RendererBackendEndFrame end_frame;

    RendererBackendUpdateGlobalUniform update_global_uniform;

    RendererBackendCreateGeometry create_geometry;
    RendererBackendDrawGeometry draw_geometry;
    RendererBackendDestroyGeometry destroy_geometry;

    RendererBackendCreateTexture create_texture;
    RendererBackendDestroyTexture destroy_texture;

    RendererBackendCreateMaterial create_material;
    RendererBackendDestroyMaterial destroy_material;
} RendererBackend;

typedef struct RenderPacket 
{
    f64 delta_time;

    u32 geometry_count;
    GeometryRenderData* geometries;
} RenderPacket;