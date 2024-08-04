#include <entry.h>
#include "game.h"
#include <core/memory.h>

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

    game->state = memory_alloc(sizeof(GameState), MEMORY_TAG_GAME);
    return true;
}