#pragma once

#include "defines.h"

typedef struct FreeListNode
{
    u64 offset;
    u64 size;
    struct FreeListNode* prev;
    struct FreeListNode* next;
} FreeListNode;

typedef struct FreeList
{
    u64 total_size;
    u64 capacity;
    FreeListNode* head;
    FreeListNode* nodes;
} FreeList;

KENZINE_API void freelist_create(u64 total_size, FreeList* out_list);
KENZINE_API void freelist_destroy(FreeList* list);

KENZINE_API bool freelist_alloc(FreeList* list, u64 size, u64* out_offset);
KENZINE_API bool freelist_free(FreeList* list, u64 size, u64 offset);

KENZINE_API void freelist_clear(FreeList* list);