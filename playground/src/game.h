#pragma once

#include <defines.h>
#include <game_defines.h>
#include <lib/math/math_defines.h>

typedef struct GameState 
{
    f64 delta_time;
    Mat4 view;
    Vec3 camera_position;
    Vec3 camera_euler;
    bool camera_view_dirty;
} GameState;

bool game_init(Game* game);
bool game_update(Game* game, f64 delta_time);
bool game_render(Game* game, f64 delta_time);
void game_resize(Game* game, i32 width, i32 height);
void game_shutdown(Game* game);