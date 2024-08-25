#include "freelist.h"

#include "core/memory.h"
#include "core/log.h"
#include <stddef.h>

void empty_node(FreeList* list, FreeListNode* node);
FreeListNode* get_free_node(FreeList* list);

void freelist_create(u64 total_size, void* nodes_memory, FreeList* out_list)
{
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
        FreeListNode* new_node = get_free_node(list);        
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
                FreeListNode* new_node = get_free_node(list);
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

bool freelist_resize(FreeList* list, u64 new_total_size, void* new_nodes_memory, void** out_old_nodes_memory)
{
    if (list == NULL || list->nodes == NULL || new_nodes_memory == NULL || list->total_size > new_total_size)
    {
        return false;
    }
    
    u64 new_capacity = (new_total_size / sizeof(FreeListNode));
    *out_old_nodes_memory = list->nodes;

    FreeListNode* old_nodes = list->nodes; // backup old nodes

    u64 old_size = list->total_size;
    u64 size_diff = new_total_size - old_size;
    list->total_size = new_total_size;
    list->capacity = new_capacity;
    list->nodes = (FreeListNode*) new_nodes_memory;
    memory_zero(list->nodes, sizeof(FreeListNode) * new_capacity);

    for (i64 i = 0; i < list->capacity; ++i)
    {
        list->nodes[i].offset = INVALID_ID;
        list->nodes[i].size = INVALID_ID;
        list->nodes[i].prev = NULL;
        list->nodes[i].next = NULL;
    }

    list->head = &list->nodes[0];

    FreeListNode* new_node = list->head;
    FreeListNode* old_node = &old_nodes[0];

    if (old_node == NULL)
    {
        // No old head, so list completely allocated. So we put head at the end of list with the diff size
        new_node->offset = old_size;
        new_node->size = size_diff;
        new_node->next = NULL;
        new_node->prev = NULL;
    }
    else 
    {
        while (old_node != NULL)
        {
            // generate new node to copy data from
            FreeListNode* node_tmp = get_free_node(list);
            node_tmp->offset = old_node->offset;
            node_tmp->size = old_node->size;
            node_tmp->prev = new_node;
            node_tmp->next = NULL;
            new_node->next = node_tmp;

            new_node = new_node->next;

            if (old_node->next != NULL)
            {
                old_node = old_node->next;
            }
            else 
            {
                // end of list
                if (old_node->offset + old_node->size == old_size)
                {
                    // old node is at the end of the list, so we need to adjust the size
                    new_node->size += size_diff;
                }
                else 
                {
                    // old node is not at the end of the list, so we need to create a new node
                    FreeListNode* new_end_node = get_free_node(list);
                    new_end_node->offset = old_size;
                    new_end_node->size = size_diff;
                    new_end_node->prev = new_node;
                    new_end_node->next = NULL;
                    new_node->next = new_end_node;
                }

                break;
            }
        }
    }

    return true;
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
    if (capacity <= 0)
    {
        capacity = 1;
    }
    return sizeof(FreeListNode) * capacity;
}

FreeListNode* get_free_node(FreeList* list)
{
    FreeListNode* new_node = NULL;
    for (i64 i = 0; i < list->capacity; i++)
    {
        if (list->nodes[i].offset == INVALID_ID)
        {
            new_node = &list->nodes[i];
            break;
        }
    }
    return new_node;
}