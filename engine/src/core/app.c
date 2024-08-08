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
} AppState;

static bool initialized = false;
static AppState state = {0};

bool app_on_event(u16 code, void* sender, void* listener, EventContext context);
bool app_on_key(u16 code, void* sender, void* listener, EventContext context);
bool app_on_resize(u16 code, void* sender, void* listener, EventContext context);

KENZINE_API bool app_init(Game* game)
{
    if (initialized)
    {
        log_error("App already initialized");
        return false;
    }

    state.game = game;

    // Subsystems
    log_init();
    input_init();

    state.running = true;
    state.suspended = false;

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
        &state.platform, 
        game->app_config.name, 
        game->app_config.width, 
        game->app_config.height, 
        game->app_config.start_x, 
        game->app_config.start_y))
    {
        log_error("Failed to initialize platform");
        return false;
    }

    if (!renderer_init(game->app_config.name, &state.platform))
    {
        log_fatal("Failed to initialize renderer");
        return false;
    }

    if (!state.game->init(state.game)) {
        log_fatal("Failed to initialize game");
        return false;
    }

    state.game->resize(state.game, game->app_config.width, game->app_config.height);

    initialized = true;

    return true;
}

KENZINE_API bool app_run(void)
{
    clock_start(&state.clock);
    clock_update(&state.clock);
    state.last_time = state.clock.elapsed_time;
    
    f64 running_time = 0.0;
    u8 frame_count = 0;
    f64 target_frame_time = 1.0 / 60.0;

    log_info(get_memory_report());

    if (!initialized)
    {
        log_error("App not initialized");
        return false;
    }

    RenderPacket packet = {0};

    while(state.running)
    {
        if(!platform_handle_messages(&state.platform))
        {
            state.running = false;
        }

        if (!state.suspended)
        {
            clock_update(&state.clock);
            f64 current_time = state.clock.elapsed_time;
            f64 delta_time = current_time - state.last_time;
            f64 frame_start_time = platform_get_absolute_time();

            if (!state.game->update(state.game, delta_time))
            {
                log_fatal("Failed to update game");
                state.running = false;
                break;
            }

            if (!state.game->render(state.game, delta_time))
            {
                log_fatal("Failed to render game");
                state.running = false;
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

            state.last_time = current_time;
        }
    }

    app_shutdown();
    return true;
}

KENZINE_API void app_shutdown(void)
{
    if (!initialized)
    {
        log_error("App not initialized");
        return;
    }

    state.game->shutdown(state.game);
    state.running = false;

    event_unsubscribe(EVENT_CODE_APPLICATION_QUIT, 0, app_on_event);
    event_unsubscribe(EVENT_CODE_KEY_PRESSED, 0, app_on_key);
    event_unsubscribe(EVENT_CODE_KEY_RELEASED, 0, app_on_key);

    event_system_shutdown();

    input_shutdown();

    renderer_shutdown();
    platform_shutdown(&state.platform);
    
    log_shutdown();
}

bool app_on_event(u16 code, void* sender, void* listener, EventContext context)
{
    switch (code)
    {
        case EVENT_CODE_APPLICATION_QUIT: 
        {
            log_info("Application quit event received");
            state.running = false;
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
    *width = state.current_width;
    *height = state.current_height;
}

bool app_on_resize(u16 code, void* sender, void* listener, EventContext context)
{
    if (code != EVENT_CODE_RESIZED)
    {
        return false;
    }

    u32 width = context.data.u32[0];
    u32 height = context.data.u32[1];

    if (width != state.current_width || height != state.current_height)
    {
        state.current_width = width;
        state.current_height = height;

        // Minimized
        if (width == 0 || height == 0)
        {
            log_info("Window minimized");
            state.suspended = true;
            return true;
        }
        else 
        {
            if (state.suspended)
            {
                log_info("Window restored");
                state.suspended = false;
            }
            
            state.game->resize(state.game, width, height);
            renderer_resize(width, height);
        }
    }

    return false;
}