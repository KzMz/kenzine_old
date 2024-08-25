#include "memory.h"
#include <string.h>
#include <stdio.h>

#define MEMORY_REPORT_SIZE 1024 * 8

typedef struct MemoryState
{
    Arena memory_arenas[MEMORY_TAG_COUNT];
    DynamicAllocator dynamic_allocator;
    void* dynamic_allocator_memory;
    MemoryAllocationType allocation_type;
} MemoryState;

static MemoryState* memory_state = NULL;

static const char* memory_strings[MEMORY_TAG_COUNT] = 
{
    "NONE\t\t",
    "GAME\t\t",
    "DYNARRAY\t",
    "INPUTDEVICE\t",
    "RENDERER\t",
    "STRING\t\t",
    "APP\t\t",
    "TEXTURE\t\t",
    "GEOMETRY\t",
    "HASHTABLE\t",
    "FREELIST\t",
    "RESOURCESYSTEM\t",
    "TEXTURESYSTEM\t",
    "MATERIALSYSTEM\t",
    "GEOMETRYSYSTEM\t",
    "MATERIALINSTANCE\t",
    "BINARY\t\t",
    "TEXT\t\t",
    "CUSTOM\t\t",
};

void memory_init(MemorySystemConfiguration config)
{
    if (memory_state != NULL)
    {
        log_error("Memory system already initialized");
        return;
    }

    u64 state_size = memory_get_state_size();
    memory_state = (MemoryState*) platform_alloc(state_size + config.dynamic_allocator_size, false);
    platform_zero_memory(memory_state, state_size + config.dynamic_allocator_size);

    if (config.arena_region_size > 0)
    {
        arena_set_region_size(config.arena_region_size);
    }

    if (config.dynamic_allocator_size > 0)
    {
        u64 nodes_size = freelist_get_nodes_size(config.dynamic_allocator_size); // can have a remainder so we do this to round it to the node size
        memory_state->dynamic_allocator_memory = platform_alloc(nodes_size, false);
        memory_dynalloc_create(config.dynamic_allocator_size, memory_state->dynamic_allocator_memory, &memory_state->dynamic_allocator);
    }
}

void memory_shutdown(void)
{
    for (u32 i = 0; i < MEMORY_TAG_COUNT; i++)
    {
        arena_clear(&memory_state->memory_arenas[i]);
    }

    memory_dynalloc_destroy(&memory_state->dynamic_allocator, true);
}

void* memory_alloc(u64 size, MemoryTag tag)
{
    // TODO: memory alignment

    switch (memory_state->allocation_type)
    {
        default:
        case MEMORY_ALLOCATION_TYPE_ARENA:
        {
            return memory_arena_alloc(&memory_state->memory_arenas[tag], size, false);
        } break;
        case MEMORY_ALLOCATION_TYPE_DYNAMIC:
        {
            // TODO: record statistics here
            return memory_dynalloc_alloc(&memory_state->dynamic_allocator, size);
        } break;
    }
}

void memory_free(void* block, u64 size, MemoryTag tag)
{
    switch (memory_state->allocation_type)
    {
        default:
        case MEMORY_ALLOCATION_TYPE_ARENA:
        {

        } break;
        case MEMORY_ALLOCATION_TYPE_DYNAMIC:
        {
            memory_dynalloc_free(&memory_state->dynamic_allocator, block, size);
        } break;
    }
}

void memory_free_all(MemoryTag tag)
{
    switch (memory_state->allocation_type)
    {
        case MEMORY_ALLOCATION_TYPE_ARENA:
        {
            memory_arena_clear(&memory_state->memory_arenas[tag]);
        } break;
        case MEMORY_ALLOCATION_TYPE_DYNAMIC:
        {

        } break;
    }
}

void memory_arena_destroy(Arena* arena)
{
    arena_clear(arena);
}

void* memory_arena_alloc(Arena* arena, u64 size, bool aligned)
{
    return arena_alloc(arena, size, aligned);
}

void memory_arena_clear(Arena* arena)
{
    arena_clear(arena);
}

bool memory_dynalloc_create(u64 size, void* nodes_memory, DynamicAllocator* out_allocator)
{
    if (size == 0)
    {
        log_error("DynamicAllocator size must be greater than 0");
        return false;
    }
    if (out_allocator == NULL)
    {
        log_error("DynamicAllocator out_allocator must not be NULL");
        return false;
    }

    if (nodes_memory == NULL)
    {
        log_error("DynamicAllocator nodes_memory must not be NULL");
        return false;
    }

    freelist_create(size, nodes_memory, &out_allocator->free_list);
    return true;
}

bool memory_dynalloc_destroy(DynamicAllocator* allocator, bool destroy_nodes)
{
    if (allocator == NULL)
    {
        log_error("DynamicAllocator allocator must not be NULL");
        return false;
    }

    if (destroy_nodes && allocator->free_list.nodes != NULL)
    {
        platform_free(allocator->free_list.nodes, false);
    }

    freelist_destroy(&allocator->free_list);
    memory_zero(allocator, sizeof(DynamicAllocator));
    return true;
}

void* memory_dynalloc_alloc(DynamicAllocator* allocator, u64 size)
{
    if (allocator == NULL)
    {
        log_error("DynamicAllocator allocator must not be NULL");
        return NULL;
    }
    if (size == 0)
    {
        log_error("DynamicAllocator requested size must be greater than 0");
        return NULL;
    }

    u64 offset = 0;
    if (!freelist_alloc(&allocator->free_list, size, &offset))
    {
        log_error("DynamicAllocator failed to allocate %llu bytes. No blocks large enough to allocate", size);
        return NULL;
    }

    return (void*) (allocator->free_list.nodes + offset);
}

bool memory_dynalloc_free(DynamicAllocator* allocator, void* block, u64 size)
{
    if (allocator == NULL)
    {
        log_error("DynamicAllocator allocator must not be NULL");
        return false;
    } 
    if (block == NULL)
    {
        log_error("DynamicAllocator block must not be NULL");
        return false;
    }
    if (size == 0)
    {
        log_error("DynamicAllocator requested size must be greater than 0");
        return false;
    }

    if ((u64) block < (u64) allocator->free_list.nodes || (u64) block > ((u64) allocator->free_list.nodes) + allocator->free_list.total_size)
    {
        log_error("DynamicAllocator block is not within the bounds of the allocator (0x%p - 0x%p)", allocator->free_list.nodes, allocator->free_list.nodes + allocator->free_list.total_size);
        return false;
    }

    u64 offset = (u64) block - (u64) allocator->free_list.nodes;
    if (!freelist_free(&allocator->free_list, size, offset))
    {
        log_error("DynamicAllocator failed to free %llu bytes at offset %llu", size, offset);
        return false;
    }

    return true;
}

void memory_zero(void* block, u64 size)
{
    platform_zero_memory(block, size);
}

void memory_copy(void* dest, const void* source, u64 size)
{
    platform_copy_memory(dest, source, size);
}

void memory_set(void* dest, i32 value, u64 size)
{
    platform_set_memory(dest, value, size);
}

char* get_memory_report(void)
{
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    char memory_report[MEMORY_REPORT_SIZE] = "System Memory Report:\n";
    u64 offset = strlen(memory_report);

    for (u32 i = 0; i < MEMORY_TAG_COUNT; ++i) 
    {
        char unit[4] = "KiB";
        char max_unit[4] = "KiB";
        u64 num_allocations = memory_state->memory_arenas[i].num_allocations;
        u64 num_dynamic_allocations = memory_state->memory_arenas[i].num_dynamic_allocations;
        f32 size = arena_get_size(&memory_state->memory_arenas[i]);
        f32 max_size = arena_get_max_size(&memory_state->memory_arenas[i]);
        
        if (size >= gib) 
        {
            size /= (f32) gib;
            unit[0] = 'G';
        } 
        else if (size >= mib) 
        {
            size /= (f32) mib;
            unit[0] = 'M';
        } 
        else if (size >= kib) 
        {
            size /= (f32) kib;
            unit[0] = 'K';
        } 
        else 
        {
            unit[0] = 'B';
            unit[1] = '\0';
        }

        if (max_size >= gib) 
        {
            max_size /= (f32) gib;
            max_unit[0] = 'G';
        } 
        else if (max_size >= mib) 
        {
            max_size /= (f32) mib;
            max_unit[0] = 'M';
        } 
        else if (max_size >= kib) 
        {
            max_size /= (f32) kib;
            max_unit[0] = 'K';
        }
        else 
        {
            max_unit[0] = 'B';
            max_unit[1] = '\0';
        }

        i32 length = snprintf(memory_report + offset, MEMORY_REPORT_SIZE - offset, "%s: %llu allocations (%llu dynamic) - %.2f%s (%.2f%s max)\n", 
            memory_strings[i], num_allocations, num_dynamic_allocations, size, unit, max_size, max_unit);
        offset += length;
    }

    return _strdup(memory_report);
}

u64 memory_get_state_size(void)
{
    return sizeof(MemoryState);
}