#pragma once

#include "defines.h"
#include "lib/arena.h"

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

    MEMORY_TAG_CUSTOM,

    MEMORY_TAG_COUNT
} MemoryTag;

KENZINE_API void memory_init(void);
KENZINE_API void memory_shutdown(void);

KENZINE_API void* memory_alloc(u64 size, MemoryTag tag);
KENZINE_API void memory_free(void* block, u64 size, MemoryTag tag);
KENZINE_API void memory_free_all(MemoryTag tag);

KENZINE_API void memory_zero(void* block, u64 size);
KENZINE_API void memory_copy(void* dest, const void* source, u64 size);
KENZINE_API void memory_set(void* dest, i32 value, u64 size);

KENZINE_API char* get_memory_report(void);