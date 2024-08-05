#pragma once

#include "renderer_defines.h"

struct Platform;

bool renderer_init(const char* app_name, struct Platform* platform);
void renderer_shutdown(void);

void renderer_resize(i32 width, i32 height);

bool renderer_draw_frame(RenderPacket* packet);