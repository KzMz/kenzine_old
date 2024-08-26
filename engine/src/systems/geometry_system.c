#include "geometry_system.h"

#include "core/log.h"
#include "core/memory.h"
#include "lib/string.h"
#include "systems/material_system.h"
#include "renderer/renderer_frontend.h"
#include "lib/containers/dyn_array.h"

#include <stddef.h>

typedef struct GeometryReference
{
    u64 reference_count;
    Geometry geometry;
    bool auto_release;
} GeometryReference;

typedef struct GeometrySystemState
{
    GeometrySystemConfig config;
    
    Geometry default_geometry;
    Geometry default_2d_geometry;

    GeometryReference* geometries;
} GeometrySystemState;

static GeometrySystemState* geometry_system_state = 0;

bool create_default_geometries(GeometrySystemState* state);
bool create_geometry(GeometrySystemState* state, GeometryConfig config, Geometry* out_geometry);
void destroy_geometry(GeometrySystemState* state, Geometry* geometry);

bool geometry_system_init(void* state, GeometrySystemConfig config)
{
    if (config.max_geometries == 0)
    {
        log_error("Invalid geometry system configuration: max_geometries must be greater than 0");
        return false;
    }

    geometry_system_state = (GeometrySystemState*) state;
    geometry_system_state->config = config;
    geometry_system_state->geometries = state + sizeof(GeometrySystemState);

    for (u32 i = 0; i < config.max_geometries; ++i)
    {
        geometry_system_state->geometries[i].geometry.id = INVALID_ID;
        geometry_system_state->geometries[i].geometry.internal_id = INVALID_ID;
        geometry_system_state->geometries[i].geometry.generation = INVALID_ID;
    }

    if (!create_default_geometries(geometry_system_state))
    {
        log_error("Failed to create default geometry");
        return false;
    }

    return true;
}

void geometry_system_shutdown(void)
{
    memory_zero(geometry_system_state->geometries, sizeof(GeometryReference) * geometry_system_state->config.max_geometries);
    memory_zero(geometry_system_state, sizeof(GeometrySystemState));
}

u64 geometry_system_get_state_size(GeometrySystemConfig config)
{
    return sizeof(GeometrySystemState) + (sizeof(GeometryReference) * config.max_geometries); 
}

Geometry* geometry_system_acquire_by_id(u64 id)
{
    if (id != INVALID_ID && geometry_system_state->geometries[id].geometry.id != INVALID_ID)
    {
        geometry_system_state->geometries[id].reference_count++;
        return &geometry_system_state->geometries[id].geometry;
    }

    return NULL;
}

Geometry* geometry_system_acquire_from_config(GeometryConfig config, bool auto_release)
{
    Geometry* geometry = NULL;
    for (u32 i = 0; i < geometry_system_state->config.max_geometries; ++i)
    {
        if (geometry_system_state->geometries[i].geometry.id != INVALID_ID) continue;

        geometry_system_state->geometries[i].auto_release = auto_release;
        geometry_system_state->geometries[i].reference_count = 1;
        geometry = &geometry_system_state->geometries[i].geometry;
        geometry->id = i;
        break;   
    }

    if (!geometry)
    {
        log_error("Failed to acquire geometry: no free slots");
        return NULL;
    }

    if (!create_geometry(geometry_system_state, config, geometry))
    {
        log_error("Failed to create geometry");
        return NULL;
    }

    return geometry;
}

void geometry_system_release(Geometry* geometry)
{
    if (geometry && geometry->id != INVALID_ID)
    {
        u32 id = geometry->id;
        GeometryReference* ref = &geometry_system_state->geometries[id];

        if (ref->geometry.id == id)
        {
            if (ref->reference_count > 0)
            {
                ref->reference_count--;
            }

            if (ref->reference_count < 1 && ref->auto_release)
            {
                destroy_geometry(geometry_system_state, &ref->geometry);
                ref->reference_count = 0;
                ref->auto_release = false;
            }
        }
        else
        {
            log_fatal("Geometry id mismatch");
        }

        return;
    }

    log_warning("Failed to release geometry: invalid geometry");
}

Geometry* geometry_system_get_default(void)
{
    if (geometry_system_state)
    {
        return &geometry_system_state->default_geometry;
    }

    return NULL;
}

Geometry* geometry_system_get_default_2d(void)
{
    if (geometry_system_state)
    {
        return &geometry_system_state->default_2d_geometry;
    }

    return NULL;
}

bool create_geometry(GeometrySystemState* state, GeometryConfig config, Geometry* out_geometry)
{
    if (!renderer_create_geometry(
        out_geometry, 
        config.vertex_count, config.vertex_size, config.vertices, 
        config.index_count, config.index_size, config.indices
    ))
    {
        state->geometries[out_geometry->id].reference_count = 0;
        state->geometries[out_geometry->id].auto_release = false;
        out_geometry->id = INVALID_ID;
        out_geometry->generation = INVALID_ID;
        out_geometry->internal_id = INVALID_ID;

        return false;
    }

    if (string_length(config.material_name) > 0)
    {
        out_geometry->material = material_system_acquire(config.material_name);
        if (out_geometry->material == NULL)
        {
            out_geometry->material = material_system_get_default();
        }
    }

    return true;
}

void destroy_geometry(GeometrySystemState* state, Geometry* geometry)
{
    renderer_destroy_geometry(geometry);
    geometry->internal_id = INVALID_ID;
    geometry->generation = INVALID_ID;
    geometry->id = INVALID_ID;

    string_empty(geometry->name);

    if (geometry->material == NULL) return;
    if (string_length(geometry->material->name) == 0) return;

    material_system_release(geometry->material->name);
    geometry->material = NULL;
}

bool create_default_geometries(GeometrySystemState* state)
{
    state->default_geometry.internal_id = INVALID_ID;
    state->default_2d_geometry.internal_id = INVALID_ID;

    Vertex3d verts[4] = {
        {
            .position = {-.5f * 10, -0.5f * 10, 0.0f},
            .texcoord = {0.0f, 0.0f},
        },
        {
            .position = {0.5f * 10, 0.5f * 10, 0.0f},
            .texcoord = {1.0f, 1.0f},
        },
        {
            .position = {-0.5f * 10, 0.5f * 10, 0.0f},
            .texcoord = {0.0f, 1.0f},
        },
        {
            .position = {0.5f * 10, -0.5f * 10, 0.0f},
            .texcoord = {1.0f, 0.0f},    
        }
    };

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    if (!renderer_create_geometry(&state->default_geometry, 4, sizeof(Vertex3d), verts, 6, sizeof(u32), indices))
    {
        log_fatal("Failed to create default geometry");
        return false;
    }

    state->default_geometry.material = material_system_get_default();

    Vertex2d verts_2d[4] = {
        {
            .position = {-.5f * 10, -0.5f * 10},
            .texcoord = {0.0f, 0.0f},
        },
        {
            .position = {0.5f * 10, 0.5f * 10},
            .texcoord = {1.0f, 1.0f},
        },
        {
            .position = {-0.5f * 10, 0.5f * 10},
            .texcoord = {0.0f, 1.0f},
        },
        {
            .position = {0.5f * 10, -0.5f * 10},
            .texcoord = {1.0f, 0.0f},    
        }
    };    

    u32 indices_2d[6] = {2, 1, 0, 3, 0, 1};

    if (!renderer_create_geometry(&state->default_2d_geometry, 4, sizeof(Vertex2d), verts_2d, 6, sizeof(u32), indices_2d))
    {
        log_fatal("Failed to create default 2d geometry");
        return false;
    }

    state->default_2d_geometry.material = material_system_get_default();

    return true;
}

GeometryConfig geometry_system_generate_plane_config(
    f32 width, f32 height, 
    u32 x_segments, u32 y_segments,
    f32 tile_x, f32 tile_y,
    const char* name,
    const char* material_name
)
{
    if (width == 0)
    {
        log_warning("Width must be greater than 0");
        width = 1;
    }
    if (height == 0)
    {
        log_warning("Height must be greater than 0");
        height = 1;
    }
    if (x_segments == 0)
    {
        log_warning("X segments must be greater than 0");
        x_segments = 1;
    }
    if (y_segments == 0)
    {
        log_warning("Y segments must be greater than 0");
        y_segments = 1;
    }
    if (tile_x == 0)
    {
        log_warning("Tile x must be greater than 0");
        tile_x = 1;
    }
    if (tile_y == 0)
    {
        log_warning("Tile y must be greater than 0");
        tile_y = 1;
    }

    GeometryConfig config;
    config.vertex_size = sizeof(Vertex3d);
    config.vertex_count = x_segments * y_segments * 4;
    config.vertices = memory_alloc(sizeof(Vertex3d) * config.vertex_count, MEMORY_TAG_GEOMETRY);
    config.index_size = sizeof(u32);
    config.index_count = x_segments * y_segments * 6;
    config.indices = memory_alloc(sizeof(u32) * config.index_count, MEMORY_TAG_GEOMETRY);

    f32 segment_width = width / x_segments;
    f32 segment_height = height / y_segments;
    f32 half_width = width * 0.5f;
    f32 half_height = height * 0.5f;
    for (u32 y = 0; y < y_segments; ++y) 
    {
        for (u32 x = 0; x < x_segments; ++x)
        {
            f32 min_x = (x * segment_width) - half_width;
            f32 min_y = (y * segment_height) - half_height;
            f32 max_x = min_x + segment_width;
            f32 max_y = min_y + segment_height;
            f32 min_uvx = (x / (f32) x_segments) * tile_x;
            f32 min_uvy = (y / (f32) y_segments) * tile_y;
            f32 max_uvx = ((x + 1) / (f32) x_segments) * tile_x;
            f32 max_uvy = ((y + 1) / (f32) y_segments) * tile_y;

            u32 v_offset = ((y * x_segments) + x) * 4;
            Vertex3d* v0 = &((Vertex3d*) config.vertices)[v_offset + 0];
            Vertex3d* v1 = &((Vertex3d*) config.vertices)[v_offset + 1];
            Vertex3d* v2 = &((Vertex3d*) config.vertices)[v_offset + 2];
            Vertex3d* v3 = &((Vertex3d*) config.vertices)[v_offset + 3];

            v0->position = (Vec3) { min_x, min_y, 0.0f };
            v0->texcoord = (Vec2) { min_uvx, min_uvy };

            v1->position = (Vec3) { max_x, max_y, 0.0f };
            v1->texcoord = (Vec2) { max_uvx, max_uvy };

            v2->position = (Vec3) { min_x, max_y, 0.0f };
            v2->texcoord = (Vec2) { min_uvx, max_uvy };

            v3->position = (Vec3) { max_x, min_y, 0.0f };
            v3->texcoord = (Vec2) { max_uvx, min_uvy };

            u32 i_offset = ((y * x_segments) + x) * 6;
            ((u32*) config.indices)[i_offset + 0] = v_offset + 0;
            ((u32*) config.indices)[i_offset + 1] = v_offset + 1;
            ((u32*) config.indices)[i_offset + 2] = v_offset + 2;
            ((u32*) config.indices)[i_offset + 3] = v_offset + 0;
            ((u32*) config.indices)[i_offset + 4] = v_offset + 3;
            ((u32*) config.indices)[i_offset + 5] = v_offset + 1;
        }
    }

    if (name && string_length(name) > 0)
    {
        string_copy_n(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    }
    else
    {
        string_copy_n(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0)
    {
        string_copy_n(config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
    }
    else
    {
        string_copy_n(config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}

GeometryConfig geometry_system_generate_cube_config(
    f32 width, f32 height, f32 depth,
    f32 tile_x, f32 tile_y,
    const char* name, 
    const char* material_name
)
{
    if (width == 0)
    {
        log_warning("Width must be greater than 0");
        width = 1;
    }
    if (height == 0)
    {
        log_warning("Height must be greater than 0");
        height = 1;
    }
    if (depth == 0)
    {
        log_warning("Depth must be greater than 0");
        depth = 1;
    }
    if (tile_x == 0)
    {
        log_warning("Tile x must be greater than 0");
        tile_x = 1;
    }
    if (tile_y == 0)
    {
        log_warning("Tile y must be greater than 0");
        tile_y = 1;
    }

    GeometryConfig config;
    config.vertex_size = sizeof(Vertex3d);
    config.vertex_count = 4 * 6;
    config.vertices = memory_alloc(sizeof(Vertex3d) * config.vertex_count, MEMORY_TAG_GEOMETRY);
    config.index_size = sizeof(u32);
    config.index_count = 6 * 6;
    config.indices = memory_alloc(sizeof(u32) * config.index_count, MEMORY_TAG_GEOMETRY);

    f32 half_width = width * 0.5f;
    f32 half_height = height * 0.5f;
    f32 half_depth = depth * 0.5f;
    f32 min_x = -half_width;
    f32 min_y = -half_height;
    f32 min_z = -half_depth;
    f32 max_x = half_width;
    f32 max_y = half_height;
    f32 max_z = half_depth;
    f32 min_uvx = 0;
    f32 min_uvy = 0;
    f32 max_uvx = tile_x;
    f32 max_uvy = tile_y;

    Vertex3d* verts = (Vertex3d*) config.vertices;

    // Front
    verts[(0 * 4) + 0] = (Vertex3d) 
    { 
        .position = { min_x, min_y, max_z },
        .normal = { 0.0f, 0.0f, 1.0f }, 
        .texcoord = { min_uvx, min_uvy } 
    };
    verts[(0 * 4) + 1] = (Vertex3d) 
    { 
        .position = { max_x, max_y, max_z },
        .normal = { 0.0f, 0.0f, 1.0f }, 
        .texcoord = { max_uvx, max_uvy } 
    };
    verts[(0 * 4) + 2] = (Vertex3d) 
    { 
        .position = { min_x, max_y, max_z },
        .normal = { 0.0f, 0.0f, 1.0f }, 
        .texcoord = { min_uvx, max_uvy } 
    };
    verts[(0 * 4) + 3] = (Vertex3d) 
    { 
        .position = { max_x, min_y, max_z },
        .normal = { 0.0f, 0.0f, 1.0f }, 
        .texcoord = { max_uvx, min_uvy } 
    };

    // Back
    verts[(1 * 4) + 0] = (Vertex3d) 
    { 
        .position = { max_x, min_y, min_z },
        .normal = { 0.0f, 0.0f, -1.0f }, 
        .texcoord = { min_uvx, min_uvy } 
    };
    verts[(1 * 4) + 1] = (Vertex3d) 
    { 
        .position = { min_x, max_y, min_z },
        .normal = { 0.0f, 0.0f, -1.0f }, 
        .texcoord = { max_uvx, max_uvy } 
    };
    verts[(1 * 4) + 2] = (Vertex3d) 
    { 
        .position = { max_x, max_y, min_z },
        .normal = { 0.0f, 0.0f, -1.0f }, 
        .texcoord = { min_uvx, max_uvy } 
    };
    verts[(1 * 4) + 3] = (Vertex3d) 
    { 
        .position = { min_x, min_y, min_z },
        .normal = { 0.0f, 0.0f, -1.0f }, 
        .texcoord = { max_uvx, min_uvy } 
    };

    // Left
    verts[(2 * 4) + 0] = (Vertex3d) 
    { 
        .position = { min_x, min_y, min_z },
        .normal = { -1.0f, 0.0f, 0.0f }, 
        .texcoord = { min_uvx, min_uvy } 
    };
    verts[(2 * 4) + 1] = (Vertex3d) 
    { 
        .position = { min_x, max_y, max_z },
        .normal = { -1.0f, 0.0f, 0.0f }, 
        .texcoord = { max_uvx, max_uvy } 
    };
    verts[(2 * 4) + 2] = (Vertex3d) 
    { 
        .position = { min_x, max_y, min_z },
        .normal = { -1.0f, 0.0f, 0.0f }, 
        .texcoord = { min_uvx, max_uvy } 
    };
    verts[(2 * 4) + 3] = (Vertex3d) 
    { 
        .position = { min_x, min_y, max_z },
        .normal = { -1.0f, 0.0f, 0.0f }, 
        .texcoord = { max_uvx, min_uvy } 
    };

    // Right
    verts[(3 * 4) + 0] = (Vertex3d) 
    { 
        .position = { max_x, min_y, max_z },
        .normal = { 1.0f, 0.0f, 0.0f }, 
        .texcoord = { min_uvx, min_uvy } 
    };
    verts[(3 * 4) + 1] = (Vertex3d) 
    { 
        .position = { max_x, max_y, min_z },
        .normal = { 1.0f, 0.0f, 0.0f }, 
        .texcoord = { max_uvx, max_uvy } 
    };
    verts[(3 * 4) + 2] = (Vertex3d) 
    { 
        .position = { max_x, max_y, max_z },
        .normal = { 1.0f, 0.0f, 0.0f }, 
        .texcoord = { min_uvx, max_uvy } 
    };
    verts[(3 * 4) + 3] = (Vertex3d) 
    { 
        .position = { max_x, min_y, min_z },
        .normal = { 1.0f, 0.0f, 0.0f }, 
        .texcoord = { max_uvx, min_uvy } 
    };

    // Bottom
    verts[(4 * 4) + 0] = (Vertex3d) 
    { 
        .position = { max_x, min_y, max_z },
        .normal = { 0.0f, -1.0f, 0.0f }, 
        .texcoord = { min_uvx, min_uvy } 
    };
    verts[(4 * 4) + 1] = (Vertex3d) 
    { 
        .position = { min_x, min_y, min_z },
        .normal = { 0.0f, -1.0f, 0.0f }, 
        .texcoord = { max_uvx, max_uvy } 
    };
    verts[(4 * 4) + 2] = (Vertex3d) 
    { 
        .position = { max_x, min_y, min_z },
        .normal = { 0.0f, -1.0f, 0.0f }, 
        .texcoord = { min_uvx, max_uvy } 
    };
    verts[(4 * 4) + 3] = (Vertex3d) 
    { 
        .position = { min_x, min_y, max_z },
        .normal = { 0.0f, -1.0f, 0.0f }, 
        .texcoord = { max_uvx, min_uvy } 
    };

    // Top
    verts[(5 * 4) + 0] = (Vertex3d) 
    { 
        .position = { min_x, max_y, max_z },
        .normal = { 0.0f, 1.0f, 0.0f }, 
        .texcoord = { min_uvx, min_uvy } 
    };
    verts[(5 * 4) + 1] = (Vertex3d) 
    { 
        .position = { max_x, max_y, min_z },
        .normal = { 0.0f, 1.0f, 0.0f }, 
        .texcoord = { max_uvx, max_uvy } 
    };
    verts[(5 * 4) + 2] = (Vertex3d) 
    { 
        .position = { min_x, max_y, min_z },
        .normal = { 0.0f, 1.0f, 0.0f }, 
        .texcoord = { min_uvx, max_uvy } 
    };
    verts[(5 * 4) + 3] = (Vertex3d) 
    { 
        .position = { max_x, max_y, max_z },
        .normal = { 0.0f, 1.0f, 0.0f }, 
        .texcoord = { max_uvx, min_uvy } 
    };

    u32* indices = (u32*) config.indices;
    for (u32 i = 0; i < 6; ++i)
    {
        u64 v_offset = i * 4;
        u64 i_offset = i * 6;

        indices[i_offset + 0] = v_offset + 0;
        indices[i_offset + 1] = v_offset + 1;
        indices[i_offset + 2] = v_offset + 2;
        indices[i_offset + 3] = v_offset + 0;
        indices[i_offset + 4] = v_offset + 3;
        indices[i_offset + 5] = v_offset + 1;
    }

    if (name && string_length(name) > 0)
    {
        string_copy_n(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    }
    else
    {
        string_copy_n(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (material_name && string_length(material_name) > 0)
    {
        string_copy_n(config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
    }
    else
    {
        string_copy_n(config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
    }

    return config;
}