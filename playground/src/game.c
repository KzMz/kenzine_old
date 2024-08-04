#include "game.h"
#include <core/log.h>

bool game_init(Game* game)
{
    log_info("Game initialized");
    return true;
}

bool game_update(Game* game, f64 delta_time)
{
    //log_info("Game updated");
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