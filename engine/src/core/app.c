#include "app.h"
#include "platform/platform.h"
#include "game_defines.h"
#include "core/memory.h"
#include "core/event.h"
#include "core/input/input.h"

typedef struct AppState
{
    Game* game;
    bool running;
    bool suspended;
    Platform platform;
    i16 current_width;
    i16 current_height;
    f64 last_time;
} AppState;

static bool initialized = false;
static AppState state = {0};

bool app_on_event(u16 code, void* sender, void* listener, EventContext context);
bool app_on_key(u16 code, void* sender, void* linster, EventContext context);

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
    log_info(get_memory_report());

    if (!initialized)
    {
        log_error("App not initialized");
        return false;
    }

    while(state.running)
    {
        if(!platform_handle_messages(&state.platform))
        {
            state.running = false;
        }

        if (!state.suspended)
        {
            if (!state.game->update(state.game, (f64) 0))
            {
                log_fatal("Failed to update game");
                state.running = false;
                break;
            }

            if (!state.game->render(state.game, (f64) 0))
            {
                log_fatal("Failed to render game");
                state.running = false;
                break;
            }

            input_update((f64) 0);
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