#pragma once

#include "defines.h"
#include "platform/platform.h"

typedef struct Region 
{
    struct Region *next;
    u64 current_size;
    u64 max_size;
    bool aligned;
    u8 data[];
} Region;

typedef struct Arena 
{
    Region* first;
    Region* last;
    u64 num_allocations;
    u64 num_dynamic_allocations;
} Arena;

Region* region_create(u64 size, bool aligned);
void region_free(Region* region);

KENZINE_API void* arena_alloc(Arena* arena, u64 size, bool aligned);
KENZINE_API void arena_clear(Arena* arena);
KENZINE_API u64 arena_get_size(Arena* arena);
KENZINE_API u64 arena_get_max_size(Arena* arena);

void arena_set_region_size(u64 size);
KENZINE_API u64 arena_get_region_size(void);