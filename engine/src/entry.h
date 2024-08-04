#pragma once

#include "core/app.h"
#include "core/log.h"
#include "core/memory.h"
#include "defines.h"
#include "game_defines.h"

// From game-specific lib
extern bool game_create(Game* game);

int main(void)
{
    // Initialize memory system
    memory_init();

    // Request game instance
    Game game = {0};
    if (!game_create(&game))
    {
        log_fatal("Failed to create game");
        return -1;
    }

    if (!game_valid(game))
    {
        log_fatal("Invalid game. Functions pointers must be set up!");
        return -2;
    }

    if (!app_init(&game))
    {
        log_warning("Failed to initialize app");
        return -3;
    }

    if (!app_run())
    {
        log_warning("Failed to run app");
        return -4;
    }

    memory_shutdown();
    return 0;
}