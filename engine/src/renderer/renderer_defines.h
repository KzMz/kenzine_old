#pragma once

#include "defines.h"
#include "lib/math/math_defines.h"
#include "resources/resource_defines.h"

#define BUILTIN_SHADER_NAME_MATERIAL "Shader.Builtin.Material"
#define BUILTIN_SHADER_NAME_UI "Shader.Builtin.UI"

struct Shader;
struct ShaderUniform;

typedef enum RendererDebugViewMode
{
    RENDERER_VIEW_MODE_DEFAULT,
    RENDERER_VIEW_MODE_LIGHTING,
    RENDERER_VIEW_MODE_NORMALS,
} RendererDebugViewMode;

typedef enum RendererBackendType
{
    RENDERER_BACKEND_TYPE_VULKAN = 0,
    RENDERER_BACKEND_TYPE_OPENGL,
    RENDERER_BACKEND_TYPE_DIRECTX,
    RENDERER_BACKEND_TYPE_WEBGPU
} RendererBackendType;

typedef struct GeometryRenderData
{
    Mat4 model;
    Geometry* geometry;
} GeometryRenderData;

struct RendererBackend;
struct Platform;

typedef enum BuiltinRenderPass
{
    BUILTIN_RENDERPASS_WORLD = 0,
    BUILTIN_RENDERPASS_UI,
} BuiltinRenderPass;

typedef bool (*RendererBackendInit)(struct RendererBackend* backend, const char* app_name);
typedef void (*RendererBackendShutdown)(struct RendererBackend* backend);
typedef void (*RendererBackendResize)(struct RendererBackend* backend, i32 width, i32 height);
typedef bool (*RendererBackendBeginFrame)(struct RendererBackend* backend, f64 delta_time);
typedef bool (*RendererBackendEndFrame)(struct RendererBackend* backend, f64 delta_time);
typedef void (*RendererBackendCreateTexture)(const u8* pixels, Texture* texture);
typedef void (*RendererBackendDestroyTexture)(Texture* texture);
typedef bool (*RendererBackendCreateGeometry)
(
    Geometry* geometry, 
    u32 vertex_count, u32 vertex_size, const void* vertices, 
    u32 index_count, u32 index_size, const void* indices
);
typedef void (*RendererBackendDrawGeometry)(GeometryRenderData data);
typedef void (*RendererBackendDestroyGeometry)(Geometry* geometry);
typedef bool (*RendererBackendBeginRenderpass)(struct RendererBackend* backend, u8 pass);
typedef bool (*RendererBackendEndRenderpass)(struct RendererBackend* backend, u8 pass);
typedef bool (*RendererBackendCreateShader)(struct Shader* shader, u8 renderpass_id, u8 stage_count, const char** stage_files, ShaderStage* stages);
typedef void (*RendererBackendDestroyShader)(struct Shader* shader);
typedef bool (*RendererBackendInitShader)(struct Shader* shader);
typedef bool (*RendererBackendUseShader)(struct Shader* shader);
typedef bool (*RendererBackendBindShaderGlobals)(struct Shader* shader);
typedef bool (*RendererBackendBindShaderInstance)(struct Shader* shader, u64 instance_id);
typedef bool (*RendererBackendApplyShaderGlobals)(struct Shader* shader);  
typedef bool (*RendererBackendApplyShaderInstance)(struct Shader* shader);
typedef bool (*RendererBackendAcquireShaderInstanceResources)(struct Shader* shader, u64* out_instance_id);
typedef bool (*RendererBackendReleaseShaderInstanceResources)(struct Shader* shader, u64 instance_id);
typedef bool (*RendererBackendSetShaderUniform)(struct Shader* shader, struct ShaderUniform* uniform, const void* value);

typedef struct RendererBackend 
{
    u64 frame_number;

    RendererBackendInit init;
    RendererBackendShutdown shutdown;

    RendererBackendResize resize;

    RendererBackendBeginFrame begin_frame;
    RendererBackendEndFrame end_frame;

    RendererBackendCreateGeometry create_geometry;
    RendererBackendDrawGeometry draw_geometry;
    RendererBackendDestroyGeometry destroy_geometry;

    RendererBackendCreateTexture create_texture;
    RendererBackendDestroyTexture destroy_texture;

    RendererBackendBeginRenderpass begin_renderpass;
    RendererBackendEndRenderpass end_renderpass;

    RendererBackendCreateShader create_shader;
    RendererBackendDestroyShader destroy_shader;
    RendererBackendInitShader init_shader;
    RendererBackendUseShader use_shader;
    RendererBackendBindShaderGlobals bind_shader_globals;
    RendererBackendBindShaderInstance bind_shader_instance;
    RendererBackendApplyShaderInstance apply_shader_instance;
    RendererBackendApplyShaderGlobals apply_shader_globals;
    RendererBackendAcquireShaderInstanceResources acquire_shader_instance_resources;
    RendererBackendReleaseShaderInstanceResources release_shader_instance_resources;
    RendererBackendSetShaderUniform set_shader_uniform;
} RendererBackend;

typedef struct RenderPacket 
{
    f64 delta_time;

    u32 geometry_count;
    GeometryRenderData* geometries;

    u32 ui_geometry_count;
    GeometryRenderData* ui_geometries;
} RenderPacket;