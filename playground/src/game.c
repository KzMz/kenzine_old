#include "game.h"
#include <core/log.h>
#include <core/input/input.h>
#include <core/memory.h>

bool game_init(Game* game)
{
    log_info("Game initialized");
    return true;
}

bool game_update(Game* game, f64 delta_time)
{
    if (input_key_was_down(KEYBOARD_DEVICE_ID, KEY_M) && input_key_up(KEYBOARD_DEVICE_ID, KEY_M))
    {
        log_debug(get_memory_report());
    }

    return true;
}

bool game_render(Game* game, f64 delta_time)
{
    //log_info("Game rendered");
    return true;
}

void game_resize(Game* game, i32 width, i32 height)
{
    log_info("Game resized");
}

void game_shutdown(Game* game)
{

}