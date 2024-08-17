#pragma once

#include "renderer/renderer_defines.h"

typedef struct TextureSystemConfig
{
    u32 max_textures;
} TextureSystemConfig;

#define DEFAULT_TEXTURE_NAME "default"

bool texture_system_init(void* state, TextureSystemConfig config);
void texture_system_shutdown(void);
u64 texture_system_get_state_size(TextureSystemConfig config);

Texture* texture_system_acquire(const char* name, bool auto_release);
void texture_system_release(const char* name);

Texture* texture_system_get_default(void);