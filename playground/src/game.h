#pragma once

#include <defines.h>
#include <game_defines.h>

typedef struct GameState 
{
    f64 delta_time;
} GameState;

bool game_init(Game* game);
bool game_update(Game* game, f64 delta_time);
bool game_render(Game* game, f64 delta_time);
void game_resize(Game* game, i32 width, i32 height);
void game_shutdown(Game* game);