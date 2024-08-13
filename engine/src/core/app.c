#include "app.h"
#include "platform/platform.h"
#include "game_defines.h"
#include "core/memory.h"
#include "core/event.h"
#include "core/input/input.h"
#include "core/clock.h"

#include "renderer/renderer_frontend.h"

typedef struct AppState
{
    Game* game;
    bool running;
    bool suspended;
    Platform platform;
    i16 current_width;
    i16 current_height;
    Clock clock;
    f64 last_time;

    void* logger_state;
    u64 log_state_size;

    void* input_state;
    u64 input_state_size;
} AppState;

static AppState* app_state = 0;

bool app_on_event(u16 code, void* sender, void* listener, EventContext context);
bool app_on_key(u16 code, void* sender, void* listener, EventContext context);
bool app_on_resize(u16 code, void* sender, void* listener, EventContext context);

KENZINE_API bool app_init(Game* game)
{
    if (game->app_state)
    {
        log_error("App already initialized");
        return false;
    }

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

    if (!event_system_init())
    {
        log_error("Failed to initialize event system");
        return false;
    }

    event_subscribe(EVENT_CODE_APPLICATION_QUIT, 0, app_on_event);
    event_subscribe(EVENT_CODE_KEY_PRESSED, 0, app_on_key);
    event_subscribe(EVENT_CODE_KEY_RELEASED, 0, app_on_key);    
    event_subscribe(EVENT_CODE_RESIZED, 0, app_on_resize);

    if (!platform_init(
        &app_state->platform, 
        game->app_config.name, 
        game->app_config.width, 
        game->app_config.height, 
        game->app_config.start_x, 
        game->app_config.start_y))
    {
        log_error("Failed to initialize platform");
        return false;
    }

    if (!renderer_init(game->app_config.name, &app_state->platform))
    {
        log_fatal("Failed to initialize renderer");
        return false;
    }

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

    RenderPacket packet = {0};

    while(app_state->running)
    {
        if(!platform_handle_messages(&app_state->platform))
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

            packet.delta_time = delta_time;
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

    event_system_shutdown();

    input_shutdown();

    renderer_shutdown();
    platform_shutdown(&app_state->platform);
    
    log_shutdown();
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