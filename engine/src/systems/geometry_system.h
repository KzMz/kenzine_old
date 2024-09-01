#pragma once

#include "renderer/renderer_defines.h"
#include "lib/math/math_defines.h"

#define DEFAULT_GEOMETRY_NAME "default"

typedef struct GeometrySystemConfig
{
    u32 max_geometries;
} GeometrySystemConfig;

typedef struct GeometryConfig
{
    u32 vertex_count;
    u32 vertex_size;
    void* vertices;
    u32 index_count;
    u32 index_size;
    void* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char material_name[MATERIAL_NAME_MAX_LENGTH];    
} GeometryConfig;

bool geometry_system_init(void* state, GeometrySystemConfig config);
void geometry_system_shutdown(void);
u64 geometry_system_get_state_size(GeometrySystemConfig config);

Geometry* geometry_system_acquire_by_id(u64 id);
Geometry* geometry_system_acquire_from_config(GeometryConfig config, bool auto_release);
void geometry_system_release(Geometry* geometry);

Geometry* geometry_system_get_default(void);
Geometry* geometry_system_get_default_2d(void);

GeometryConfig geometry_system_generate_plane_config(
    f32 width, f32 height, 
    u32 x_segments, u32 y_segments,
    f32 tile_x, f32 tile_y,
    const char* name,
    const char* material_name
);

GeometryConfig geometry_system_generate_cube_config(
    f32 width, f32 height, f32 depth,
    f32 tile_x, f32 tile_y,
    const char* name, 
    const char* material_name
);

void geometry_system_config_destroy(GeometryConfig* config);