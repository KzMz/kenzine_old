#pragma once

#include "renderer_defines.h"

struct Platform;

bool renderer_init(void* state, const char* app_name);
void renderer_shutdown(void);

void renderer_resize(i32 width, i32 height);

bool renderer_draw_frame(RenderPacket* packet);

u64 renderer_get_state_size(void);

// TODO: remove it when not needed anymore
KENZINE_API void renderer_set_view(Mat4 view);