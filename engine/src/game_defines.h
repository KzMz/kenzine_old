#pragma once 

#include "core/app.h"

typedef bool (*GameInit)(struct Game* game);
typedef bool (*GameUpdate)(struct Game* game, f64 delta_time);
typedef bool (*GameRender)(struct Game* game, f64 delta_time);
typedef void (*GameResize)(struct Game* game, i32 width, i32 height);
typedef void (*GameShutdown)(struct Game* game);

typedef struct Game
{
    AppConfig app_config;

    GameInit init;
    GameUpdate update;
    GameRender render;
    GameResize resize;
    GameShutdown shutdown;

    void* state;

    void* app_state;
} Game;

#define game_valid(game) (game.init && game.update && game.render && game.resize && game.shutdown)