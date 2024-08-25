#pragma once

#include "defines.h"
#include "lib/memory/arena.h"
#include "lib/memory/freelist.h"

#define ARENA_REGION_SIZE 10 * 1024

typedef enum MemoryTag 
{
    MEMORY_TAG_NONE = 0,
    MEMORY_TAG_GAME,
    MEMORY_TAG_DYNARRAY,
    MEMORY_TAG_INPUTDEVICE,
    MEMORY_TAG_RENDERER,
    MEMORY_TAG_STRING,
    MEMORY_TAG_APP,
    MEMORY_TAG_TEXTURE,
    MEMORY_TAG_GEOMETRY,
    MEMORY_TAG_HASHTABLE,
    MEMORY_TAG_FREELIST,
    MEMORY_TAG_RESOURCESYSTEM,
    MEMORY_TAG_TEXTURESYSTEM,
    MEMORY_TAG_MATERIALSYSTEM,
    MEMORY_TAG_GEOMETRYSYSTEM,

    MEMORY_TAG_MATERIALINSTANCE,
    MEMORY_TAG_BINARY,
    MEMORY_TAG_TEXT,

    MEMORY_TAG_CUSTOM,

    MEMORY_TAG_COUNT
} MemoryTag;

// TODO: implement this while maintaining the possibility to call one or the other independently
typedef enum MemoryAllocationType
{
    MEMORY_ALLOCATION_TYPE_ARENA,
    MEMORY_ALLOCATION_TYPE_DYNAMIC,
} MemoryAllocationType;

typedef struct DynamicAllocator
{
    FreeList free_list;
} DynamicAllocator;

typedef struct MemorySystemConfiguration
{
    MemoryAllocationType allocation_type;
    u64 arena_region_size;
    u64 dynamic_allocator_size;
} MemorySystemConfiguration;

KENZINE_API void memory_init(MemorySystemConfiguration config);
KENZINE_API void memory_shutdown(void);

KENZINE_API void* memory_alloc(u64 size, MemoryTag tag);
KENZINE_API void memory_free(void* block, u64 size, MemoryTag tag);
KENZINE_API void memory_free_all(MemoryTag tag);

// Arena
KENZINE_API void memory_arena_destroy(Arena* arena);
KENZINE_API void* memory_arena_alloc(Arena* arena, u64 size, bool aligned);
KENZINE_API void memory_arena_clear(Arena* arena); 

// Dynamic allocation
KENZINE_API bool memory_dynalloc_create(u64 size, void* nodes_memory, DynamicAllocator* out_allocator);
KENZINE_API bool memory_dynalloc_destroy(DynamicAllocator* allocator, bool destroy_nodes);
KENZINE_API void* memory_dynalloc_alloc(DynamicAllocator* allocator, u64 size);
KENZINE_API bool memory_dynalloc_free(DynamicAllocator* allocator, void* block, u64 size);

KENZINE_API void memory_zero(void* block, u64 size);
KENZINE_API void memory_copy(void* dest, const void* source, u64 size);
KENZINE_API void memory_set(void* dest, i32 value, u64 size);

KENZINE_API char* get_memory_report(void);

u64 memory_get_state_size(void);