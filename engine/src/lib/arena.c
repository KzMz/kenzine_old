#include "arena.h"
#include "core/log.h"
#include "core/asserts.h"

Region* region_create(u64 size, bool aligned)
{
    const u64 total_size = size + sizeof(Region);
    Region* region = platform_alloc(total_size, aligned);
    kz_assert_msg(region != NULL, "Failed to allocate memory for region");

    platform_zero_memory(region, total_size);

    region->next = NULL;
    region->current_size = 0;
    region->max_size = size;
    region->aligned = aligned;
    return region;
}

void region_free(Region* region)
{
    platform_free(region, region->aligned);
}

void* arena_alloc(Arena* arena, u64 size, bool aligned)
{
    if (arena->last == NULL)
    {
        kz_assert(arena->first == NULL);
        u64 region_size = size > REGION_DEFAULT_SIZE ? size : REGION_DEFAULT_SIZE;
        arena->last = region_create(region_size, aligned);
        arena->last->aligned = aligned;
        arena->first = arena->last;
        arena->num_dynamic_allocations++;
    }

    while (arena->last->current_size + size > arena->last->max_size && arena->last->next != NULL)
    {
        arena->last = arena->last->next;
    }
    
    if (arena->last->current_size + size > arena->last->max_size)
    {
        kz_assert(arena->last->next == NULL);
        u64 region_size = size > REGION_DEFAULT_SIZE ? size : REGION_DEFAULT_SIZE;
        arena->last->next = region_create(region_size, aligned);
        arena->last = arena->last->next;
        arena->last->aligned = aligned;
        arena->num_dynamic_allocations++;
    }

    u8* result = arena->last->data + arena->last->current_size;
    arena->last->current_size += size;
    arena->num_allocations++;
    return result;
}

void arena_clear(Arena* arena)
{
    Region* region = arena->first;
    while (region != NULL)
    {
        Region* next = region->next;
        region_free(region);
        region = next;
    }
    arena->first = NULL;
    arena->last = NULL;
}

u64 arena_get_size(Arena* arena)
{
    u64 size = 0;
    Region* region = arena->first;
    while (region != NULL)
    {
        size += region->current_size;
        region = region->next;
    }
    return size;
}

u64 arena_get_max_size(Arena* arena)
{
    u64 size = 0;
    Region* region = arena->first;
    while (region != NULL)
    {
        size += region->max_size;
        region = region->next;
    }
    return size;
}

void* arena_alloc(Arena* arena, u64 size, bool aligned);
void* arena_alloc_aligned(Arena* arena, u64 size, u64 alignment);
void arena_clear(Arena* arena);