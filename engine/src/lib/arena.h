#pragma once

#include "defines.h"
#include "platform/platform.h"

#define REGION_DEFAULT_SIZE (8 * 1024)

typedef struct Region 
{
    struct Region *next;
    u64 current_size;
    u64 max_size;
    bool aligned;
    u8* data;
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

void* arena_alloc(Arena* arena, u64 size, bool aligned);
void arena_clear(Arena* arena);
u64 arena_get_size(Arena* arena);
u64 arena_get_max_size(Arena* arena);