#include <entry.h>
#include "game.h"
#include <core/memory.h>
#include <stddef.h>

bool game_create(Game* game)
{
    game->app_config.name = "Kenzine Playground";
    game->app_config.width = 1200;
    game->app_config.height = 720;
    game->app_config.start_x = 100;
    game->app_config.start_y = 100;

    game->init = game_init;
    game->update = game_update;
    game->render = game_render;
    game->resize = game_resize;
    game->shutdown = game_shutdown;

    game->state = NULL;
    game->state_size = sizeof(GameState);
    game->app_state = NULL;
    return true;
}