#include "memory.h"
#include <string.h>
#include <stdio.h>

static Arena memory_arenas[MEMORY_TAG_COUNT] = {0};

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
    "CUSTOM\t\t",
};

KENZINE_API void memory_init(void)
{
    // Nothing to do here with arenas
}

KENZINE_API void memory_shutdown(void)
{
    for (u32 i = 0; i < MEMORY_TAG_COUNT; i++)
    {
        arena_clear(&memory_arenas[i]);
    }
}

KENZINE_API void* memory_alloc(u64 size, MemoryTag tag)
{
    // TODO: memory alignment
    return arena_alloc(&memory_arenas[tag], size, false);
}

KENZINE_API void memory_free(void* block, u64 size, MemoryTag tag)
{
    // Nothing to do here with arenas
    (void) block;
    (void) size;
    (void) tag;
}

KENZINE_API void memory_free_all(MemoryTag tag)
{
    arena_clear(&memory_arenas[tag]);
}

KENZINE_API void memory_zero(void* block, u64 size)
{
    platform_zero_memory(block, size);
}

KENZINE_API void memory_copy(void* dest, const void* source, u64 size)
{
    platform_copy_memory(dest, source, size);
}

KENZINE_API void memory_set(void* dest, i32 value, u64 size)
{
    platform_set_memory(dest, value, size);
}

KENZINE_API char* get_memory_report(void)
{
    const u64 gib = 1024 * 1024 * 1024;
    const u64 mib = 1024 * 1024;
    const u64 kib = 1024;

    const u32 memory_report_size = 1024 * 8;
    char memory_report[memory_report_size] = "System Memory Report:\n";
    u64 offset = strlen(memory_report);

    for (u32 i = 0; i < MEMORY_TAG_COUNT; ++i) 
    {
        char unit[4] = "KiB";
        char max_unit[4] = "KiB";
        u64 num_allocations = memory_arenas[i].num_allocations;
        u64 num_dynamic_allocations = memory_arenas[i].num_dynamic_allocations;
        f32 size = arena_get_size(&memory_arenas[i]);
        f32 max_size = arena_get_max_size(&memory_arenas[i]);
        
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

        i32 length = snprintf(memory_report + offset, memory_report_size - offset, "%s: %llu allocations (%llu dynamic) - %.2f%s (%.2f%s max)\n", 
            memory_strings[i], num_allocations, num_dynamic_allocations, size, unit, max_size, max_unit);
        offset += length;
    }

    return _strdup(memory_report);
}