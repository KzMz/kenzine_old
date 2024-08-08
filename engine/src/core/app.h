#pragma once

#include "defines.h"

struct Game;

typedef struct AppConfig
{
    char* name;
    i16 width;
    i16 height;
    i16 start_x;
    i16 start_y;
} AppConfig;

KENZINE_API bool app_init(struct Game* game);
KENZINE_API bool app_run(void);
KENZINE_API void app_shutdown(void);

void app_get_framebuffer_size(u32* width, u32* height);