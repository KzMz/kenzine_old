#pragma once

#include "renderer_defines.h"

struct Platform;

bool renderer_backend_create(RendererBackendType type, struct Platform* platform, RendererBackend* out_backend);
void renderer_backend_destroy(RendererBackend* backend);