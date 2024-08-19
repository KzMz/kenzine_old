#pragma once

#include "defines.h"
#include "core/memory.h"
#include "resources/resource_defines.h"

struct ResourceLoader;

bool resource_unload(struct ResourceLoader* self, Resource* resource, MemoryTag tag);