#include "freelist.h"

#include "core/memory.h"
#include "core/log.h"
#include <stddef.h>

void empty_node(FreeList* list, FreeListNode* node);

void freelist_create(u64 total_size, void* nodes_memory, FreeList* out_list)
{
    const u32 min_entries = 8;
    u64 min_memory = (sizeof(FreeList) + sizeof(FreeListNode) * min_entries);
    if (total_size < min_memory)
    {
        log_error("FreeList: total size is too small. Minimum size is %llu bytes", min_memory);
        return;
    }

    u64 capacity = (total_size / sizeof(FreeListNode));
    out_list->total_size = total_size;
    out_list->capacity = capacity;

    out_list->nodes = (FreeListNode*) nodes_memory;
    memory_zero(out_list->nodes, sizeof(FreeListNode) * capacity);

    out_list->head = &out_list->nodes[0];
    out_list->head->offset = 0;
    out_list->head->size = total_size;
    out_list->head->prev = NULL;
    out_list->head->next = NULL;

    for (u64 i = 1; i < capacity; i++)
    {
        out_list->nodes[i].offset = INVALID_ID;
        out_list->nodes[i].size = INVALID_ID;
        out_list->nodes[i].prev = NULL;
        out_list->nodes[i].next = NULL;
    }   
}

void freelist_destroy(FreeList* list)
{
    if (list == NULL)
    {
        return;
    }
    
    list->nodes = NULL;
    memory_zero(list, sizeof(FreeList));
}

bool freelist_alloc(FreeList* list, u64 size, u64* out_offset)
{
    if (list == NULL || list->nodes == NULL || out_offset == NULL)
    {
        return false;
    }

    FreeListNode* node = list->head;

    while (node != NULL)
    {
        if (node->size == size)
        {
            // Exact match
            *out_offset = node->offset;
            if (node->prev != NULL)
            {
                node->prev->next = node->next;
                empty_node(list, node);
            }
            else 
            {
                FreeListNode* next = node->next;
                empty_node(list, list->head);
                list->head = next;
            }
            return true;
        }
        else if (node->size > size)
        {
            *out_offset = node->offset;
            node->offset += size;
            node->size -= size;
            return true;
        }

        node = node->next;
    }
    return false;
}

bool freelist_free(FreeList* list, u64 size, u64 offset)
{
    if (list == NULL || list->nodes == NULL || size == 0)
    {
        return false;
    }

    FreeListNode* node = list->head;

    if (node == NULL)
    {
        // List is full so we need to create a new node
        FreeListNode* new_node = NULL;
        for (u64 i = 0; i < list->capacity; i++)
        {
            if (list->nodes[i].offset == INVALID_ID)
            {
                new_node = &list->nodes[i];
                break;
            }
        }
        
        new_node->offset = offset;
        new_node->size = size;
        new_node->prev = NULL;
        new_node->next = NULL;
        list->head = new_node;
        return true;
    } 
    else 
    {
        while (node != NULL)
        {
            if (node->offset == offset)
            {
                node->size += size;
                
                // Check to see if this overlaps with the next node, merging them in case
                if (node->next != NULL && node->next->offset == node->offset + node->size)
                {
                    FreeListNode* next = node->next;
                    node->size += next->size;
                    node->next = next->next;
                    empty_node(list, next);
                }
                // If not, we are good
                return true;
            }
            else if (node->offset > offset)
            {
                // we are over what we need so we create a new node
                FreeListNode* new_node = NULL;
                for (u64 i = 0; i < list->capacity; i++)
                {
                    if (list->nodes[i].offset == INVALID_ID)
                    {
                        new_node = &list->nodes[i];
                        break;
                    }
                }

                if (new_node == NULL)
                {
                    log_error("FreeList: no more space for new node");
                    return false;
                }

                new_node->offset = offset;
                new_node->size = size;

                if (node->prev != NULL)
                {
                    node->prev->next = new_node;
                    new_node->next = node;
                    new_node->prev = node->prev;
                    node->prev = new_node;
                }
                else 
                {
                    new_node->next = node;
                    new_node->prev = NULL;
                    node->prev = new_node;
                    list->head = new_node;
                }

                // check next node to see if it can be merged
                if (new_node->next != NULL && new_node->offset + new_node->size == new_node->next->offset)
                {
                    new_node->size += new_node->next->size;
                    FreeListNode* to_empty = new_node->next;
                    new_node->next = to_empty->next;
                    empty_node(list, to_empty);
                }

                // check prev node to see if it can be merged
                if (new_node->prev != NULL && new_node->prev->offset + new_node->prev->size == new_node->offset)
                {
                    new_node->prev->size += new_node->size;
                    
                    FreeListNode* to_empty = new_node;
                    new_node->prev->next = to_empty->next;
                    
                    if (new_node->next != NULL)
                    {
                        new_node->next->prev = new_node->prev;
                    }

                    empty_node(list, to_empty);
                }

                return true;
            }

            node = node->next;
        }
    }

    return false;
}

void freelist_clear(FreeList* list)
{
    if (list == NULL || list->nodes == NULL)
    {
        return;
    }

    for (u64 i = 0; i < list->capacity; i++)
    {
        list->nodes[i].offset = INVALID_ID;
        list->nodes[i].size = INVALID_ID;
        list->nodes[i].prev = NULL;
        list->nodes[i].next = NULL;
    }

    list->head->offset = 0;
    list->head->size = list->total_size;
    list->head->prev = NULL;
    list->head->next = NULL;
}

void empty_node(FreeList* list, FreeListNode* node)
{
    node->offset = INVALID_ID;
    node->size = INVALID_ID;
    node->prev = NULL;
    node->next = NULL;
}

u64 freelist_get_nodes_size(u64 total_size)
{
    u64 capacity = (total_size / sizeof(FreeListNode)); 
    return sizeof(FreeListNode) * capacity;
}