#include "app.h"
#include "platform/platform.h"
#include "game_defines.h"
#include "core/memory.h"
#include "core/event.h"
#include "core/input/input.h"
#include "core/clock.h"
#include "lib/string.h"

#include "renderer/renderer_frontend.h"

#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"

#include "lib/math/math_defines.h"
#include "lib/math/mat4.h"

typedef struct AppState
{
    Game* game;
    bool running;
    bool suspended;
    i16 current_width;
    i16 current_height;
    Clock clock;
    f64 last_time;

    void* logger_state;
    u64 log_state_size;

    void* input_state;
    u64 input_state_size;

    void* event_state;
    u64 event_state_size;

    void* platform_state;
    u64 platform_state_size;

    void* resource_system_state;
    u64 resource_system_state_size;

    void* renderer_state;
    u64 renderer_state_size;

    void* texture_system_state;
    u64 texture_system_state_size;

    void* material_system_state;
    u64 material_system_state_size;

    void* geometry_system_state;
    u64 geometry_system_state_size;

    Geometry* test_geometry;
    Geometry* test_ui_geometry;
} AppState;

static AppState* app_state = 0;

bool app_on_event(u16 code, void* sender, void* listener, EventContext context);
bool app_on_key(u16 code, void* sender, void* listener, EventContext context);
bool app_on_resize(u16 code, void* sender, void* listener, EventContext context);

bool event_on_debug(u16 code, void* sender, void* listener, EventContext context);

KENZINE_API bool app_init(Game* game)
{
    if (game->app_state)
    {
        log_error("App already initialized");
        return false;
    }

    MemorySystemConfiguration config = {0};
    config.arena_region_size = 10 * 1024;
    config.dynamic_allocator_size = 0;
    config.allocation_type = MEMORY_ALLOCATION_TYPE_ARENA;
    // Initialize memory system
    memory_init(config);

    game->state = memory_alloc(game->state_size, MEMORY_TAG_GAME);

    game->app_state = memory_alloc(sizeof(AppState), MEMORY_TAG_APP);
    app_state = game->app_state;
    app_state->game = game;
    app_state->running = true;
    app_state->suspended = false;

    // Log subsystem
    app_state->log_state_size = log_state_size();
    void* logger_state = memory_alloc(app_state->log_state_size, MEMORY_TAG_APP);
    app_state->logger_state = logger_state;

    log_init(logger_state);

    // Input subsystem
    app_state->input_state_size = input_get_state_size();
    void* input_state = memory_alloc(app_state->input_state_size, MEMORY_TAG_APP);
    app_state->input_state = input_state;
    input_init(input_state);

    // Event subsystem
    app_state->event_state_size = event_system_get_state_size();
    void* event_state = memory_alloc(app_state->event_state_size, MEMORY_TAG_APP);
    app_state->event_state = event_state;
    if (!event_system_init(event_state))
    {
        log_error("Failed to initialize event system");
        return false;
    }

    event_subscribe(EVENT_CODE_APPLICATION_QUIT, 0, app_on_event);
    event_subscribe(EVENT_CODE_KEY_PRESSED, 0, app_on_key);
    event_subscribe(EVENT_CODE_KEY_RELEASED, 0, app_on_key);    
    event_subscribe(EVENT_CODE_RESIZED, 0, app_on_resize);

    event_subscribe(EVENT_CODE_DEBUG0, 0, event_on_debug);

    // Platform subsystem
    app_state->platform_state_size = platform_get_state_size();
    void* platform_state = memory_alloc(app_state->platform_state_size, MEMORY_TAG_APP);
    app_state->platform_state = platform_state;
    if (!platform_init(
        platform_state, 
        game->app_config.name, 
        game->app_config.width, 
        game->app_config.height, 
        game->app_config.start_x, 
        game->app_config.start_y))
    {
        log_error("Failed to initialize platform");
        return false;
    }

    // Resource subsystem
    ResourceSystemConfig resource_config = {0};
    resource_config.max_loaders = 32;
    resource_config.asset_base_path = "../assets";
    app_state->resource_system_state_size = resource_system_get_state_size(resource_config);
    void* resource_system_state = memory_alloc(app_state->resource_system_state_size, MEMORY_TAG_APP);
    app_state->resource_system_state = resource_system_state;
    if (!resource_system_init(resource_system_state, resource_config))
    {
        log_error("Failed to initialize resource system");
        return false;
    }

    // Renderer subsystem
    app_state->renderer_state_size = renderer_get_state_size();
    void* renderer_state = memory_alloc(app_state->renderer_state_size, MEMORY_TAG_APP);
    app_state->renderer_state = renderer_state;
    if (!renderer_init(renderer_state, game->app_config.name))
    {
        log_fatal("Failed to initialize renderer");
        return false;
    }

    // Texture system
    TextureSystemConfig texture_config = {0};
    texture_config.max_textures = 65536;
    void* texture_system_state = memory_alloc(texture_system_get_state_size(texture_config), MEMORY_TAG_TEXTURESYSTEM);
    app_state->texture_system_state = texture_system_state;
    if (!texture_system_init(texture_system_state, texture_config))
    {
        log_fatal("Failed to initialize texture system");
        return false;
    }

    // Material system
    MaterialSystemConfig material_config = {0};
    material_config.max_materials = 4096;
    void* material_system_state = memory_alloc(material_system_get_state_size(material_config), MEMORY_TAG_MATERIALSYSTEM);
    app_state->material_system_state = material_system_state;
    if (!material_system_init(material_system_state, material_config))
    {
        log_fatal("Failed to initialize material system");
        return false;
    }

    // Geometry system
    GeometrySystemConfig geometry_config = {0};
    geometry_config.max_geometries = 4096;
    void* geometry_system_state = memory_alloc(geometry_system_get_state_size(geometry_config), MEMORY_TAG_GEOMETRYSYSTEM);
    app_state->geometry_system_state = geometry_system_state;
    if (!geometry_system_init(geometry_system_state, geometry_config))
    {
        log_fatal("Failed to initialize geometry system");
        return false;
    }

    GeometryConfig plane_config = geometry_system_generate_plane_config(10.0f, 10.0f, 5, 5, 2.0f, 2.0f, "test geometry", "test_material");
    app_state->test_geometry = geometry_system_acquire_from_config(plane_config, true);

    memory_free(plane_config.vertices, sizeof(Vertex3d) * plane_config.vertex_count, MEMORY_TAG_APP);
    memory_free(plane_config.indices, sizeof(u32) * plane_config.index_count, MEMORY_TAG_APP);

    GeometryConfig ui_config;
    ui_config.vertex_count = 4;
    ui_config.vertex_size = sizeof(Vertex2d);
    ui_config.index_count = 6;
    ui_config.index_size = sizeof(u32);
    string_copy_n(ui_config.name, "test_ui_geometry", GEOMETRY_NAME_MAX_LENGTH);
    string_copy_n(ui_config.material_name, "test_ui_material", MATERIAL_NAME_MAX_LENGTH);

    const f32 f = 512.0f;
    Vertex2d uiverts[4] = {
        { { 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { f, f }, { 1.0f, 1.0f } },
        { { 0, f }, { 0.0f, 1.0f } },
        { { f, 0 }, { 1.0f, 0.0f } }
    };
    ui_config.vertices = uiverts;

    u32 uiinds[6] = { 2, 1, 0, 3, 0, 1 };
    ui_config.indices = uiinds;

    app_state->test_ui_geometry = geometry_system_acquire_from_config(ui_config, true);

    if (!app_state->game->init(app_state->game)) {
        log_fatal("Failed to initialize game");
        return false;
    }

    app_state->game->resize(app_state->game, game->app_config.width, game->app_config.height);

    return true;
}

KENZINE_API bool app_run(void)
{
    clock_start(&app_state->clock);
    clock_update(&app_state->clock);
    app_state->last_time = app_state->clock.elapsed_time;
    
    f64 running_time = 0.0;
    u8 frame_count = 0;
    f64 target_frame_time = 1.0 / 60.0;

    log_info(get_memory_report());

    if (!app_state)
    {
        log_error("App not initialized");
        return false;
    }

    while(app_state->running)
    {
        if(!platform_handle_messages())
        {
            app_state->running = false;
        }

        if (!app_state->suspended)
        {
            clock_update(&app_state->clock);
            f64 current_time = app_state->clock.elapsed_time;
            f64 delta_time = current_time - app_state->last_time;
            f64 frame_start_time = platform_get_absolute_time();

            if (!app_state->game->update(app_state->game, delta_time))
            {
                log_fatal("Failed to update game");
                app_state->running = false;
                break;
            }

            if (!app_state->game->render(app_state->game, delta_time))
            {
                log_fatal("Failed to render game");
                app_state->running = false;
                break;
            }

            RenderPacket packet = {0};
            packet.delta_time = delta_time;
            
            GeometryRenderData render_data = {0};
            render_data.geometry = app_state->test_geometry;
            render_data.model = mat4_identity();

            packet.geometries = &render_data;
            packet.geometry_count = 1;

            GeometryRenderData ui_render_data = {0};
            ui_render_data.geometry = app_state->test_ui_geometry;
            ui_render_data.model = mat4_translation((Vec3) { 0, 0, 0 });

            packet.ui_geometry_count = 1;
            packet.ui_geometries = &ui_render_data;

            renderer_draw_frame(&packet);

            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;

            f64 remaining_time = target_frame_time - frame_elapsed_time;
            if (remaining_time > 0.0) 
            {
                u64 remaining_ms = (u64)(remaining_time * 1000.0);

                bool limit_frames = false;
                if (remaining_ms > 0 && limit_frames)
                {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;
            }

            input_update(delta_time);

            app_state->last_time = current_time;
        }
    }

    app_shutdown();
    return true;
}

KENZINE_API void app_shutdown(void)
{
    if (!app_state)
    {
        log_error("App not initialized");
        return;
    }

    app_state->game->shutdown(app_state->game);
    app_state->running = false;

    event_unsubscribe(EVENT_CODE_APPLICATION_QUIT, 0, app_on_event);
    event_unsubscribe(EVENT_CODE_KEY_PRESSED, 0, app_on_key);
    event_unsubscribe(EVENT_CODE_KEY_RELEASED, 0, app_on_key);

    event_unsubscribe(EVENT_CODE_DEBUG0, 0, event_on_debug);

    input_shutdown();

    geometry_system_shutdown();
    material_system_shutdown();
    texture_system_shutdown();

    event_system_shutdown();

    renderer_shutdown();

    resource_system_shutdown();

    platform_shutdown();
    
    log_shutdown();

    memory_shutdown();
}

bool app_on_event(u16 code, void* sender, void* listener, EventContext context)
{
    switch (code)
    {
        case EVENT_CODE_APPLICATION_QUIT: 
        {
            log_info("Application quit event received");
            app_state->running = false;
            return true;
        }
    }

    return false;
}

bool app_on_key(u16 code, void* sender, void* linster, EventContext context)
{
    if (code == EVENT_CODE_KEY_PRESSED)
    {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE)
        {
            event_trigger(EVENT_CODE_APPLICATION_QUIT, 0, (EventContext) {0});
            return true;
        }
    }

    return false;
}

void app_get_framebuffer_size(u32* width, u32* height)
{
    *width = app_state->current_width;
    *height = app_state->current_height;
}

bool app_on_resize(u16 code, void* sender, void* listener, EventContext context)
{
    if (code != EVENT_CODE_RESIZED)
    {
        return false;
    }

    u32 width = context.data.u32[0];
    u32 height = context.data.u32[1];

    if (width != app_state->current_width || height != app_state->current_height)
    {
        app_state->current_width = width;
        app_state->current_height = height;

        // Minimized
        if (width == 0 || height == 0)
        {
            log_info("Window minimized");
            app_state->suspended = true;
            return true;
        }
        else 
        {
            if (app_state->suspended)
            {
                log_info("Window restored");
                app_state->suspended = false;
            }
            
            app_state->game->resize(app_state->game, width, height);
            renderer_resize(width, height);
        }
    }

    return false;
}

bool event_on_debug(u16 code, void* sender, void* listener, EventContext context)
{
    return true;
}