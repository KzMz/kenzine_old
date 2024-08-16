#include "hash_table.h"

#include "core/memory.h"
#include "core/log.h"
#include <stddef.h>

u64 hash_name(const char* name, u64 capacity)
{
    static const u64 multiplier = 97; // Prime for try to avoid collisions

    unsigned const char* ch;
    u64 hash = 0;

    for (ch = (unsigned const char*)name; *ch; ch++)
    {
        hash = multiplier * hash + *ch;
    }

    return hash % capacity;
}

void _hashtable_create(u64 capacity, u64 element_size, bool is_pointer, HashTable* out_table)
{
    if (out_table == NULL)
    {
        log_error("HashTable: out_table is NULL");
        return;
    }

    const u64 data_size = capacity * element_size;
    if (data_size == 0)
    {
        log_error("HashTable: data_size is 0");
        return;
    }

    out_table->data = memory_alloc(data_size, MEMORY_TAG_HASHTABLE);
    memory_zero(out_table->data, data_size);

    out_table->header.capacity = capacity;
    out_table->header.element_size = element_size;
    out_table->header.is_pointer = is_pointer;
}

void _hashtable_destroy(HashTable* table)
{
    if (table == NULL)
    {
        log_error("HashTable: table is NULL");
        return;
    }

    memory_free(table->data, table->header.capacity * table->header.element_size, MEMORY_TAG_HASHTABLE);
    memory_zero(table, sizeof(HashTable));
}

bool _hashtable_set_value(HashTable* table, const char* key, void* value)
{
    if (table == NULL)
    {
        log_error("HashTable: table is NULL");
        return false;
    }
    if (key == NULL)
    {
        log_error("HashTable: key is NULL");
        return false;
    }
    if (value == NULL)
    {
        log_error("HashTable: value is NULL");
        return false;
    }
    if (table->header.is_pointer)
    {
        log_error("HashTable: table is a pointer table");
        return false;
    }

    const u64 hash = hash_name(key, table->header.capacity);
    memory_copy(table->data + (table->header.element_size * hash), value, table->header.element_size);
    return true;
}

bool _hashtable_set_pointer(HashTable* table, const char* key, void** value)
{
    if (table == NULL)
    {
        log_error("HashTable: table is NULL");
        return false;
    }
    if (key == NULL)
    {
        log_error("HashTable: key is NULL");
        return false;
    }
    if (value == NULL)
    {
        log_error("HashTable: value is NULL");
        return false;
    }
    if (!table->header.is_pointer)
    {
        log_error("HashTable: table is not a pointer table");
        return false;
    }

    const u64 hash = hash_name(key, table->header.capacity);
    ((void**) table->data)[hash] = value == NULL ? NULL : *value;
    return true;
}

bool _hashtable_get_value(HashTable* table, const char* key, void* out_value)
{
    if (table == NULL)
    {
        log_error("HashTable: table is NULL");
        return false;
    }
    if (key == NULL)
    {
        log_error("HashTable: key is NULL");
        return false;
    }
    if (out_value == NULL)
    {
        log_error("HashTable: out_value is NULL");
        return false;
    }
    if (table->header.is_pointer)
    {
        log_error("HashTable: table is a pointer table");
        return false;
    }

    u64 hash = hash_name(key, table->header.capacity);
    memory_copy(out_value, table->data + (table->header.element_size * hash), table->header.element_size);
    return true;
}

bool _hashtable_get_pointer(HashTable* table, const char* key, void** out_value)
{
    if (table == NULL)
    {
        log_error("HashTable: table is NULL");
        return false;
    }
    if (key == NULL)
    {
        log_error("HashTable: key is NULL");
        return false;
    }
    if (out_value == NULL)
    {
        log_error("HashTable: out_value is NULL");
        return false;
    }
    if (!table->header.is_pointer)
    {
        log_error("HashTable: table is not a pointer table");
        return false;
    }

    u64 hash = hash_name(key, table->header.capacity);
    *out_value = ((void**) table->data)[hash];
    return *out_value != NULL;
}

bool _hashtable_fill_with_value(HashTable* table, void* value)
{
    if (table == NULL)
    {
        log_error("HashTable: table is NULL");
        return false;
    }
    if (value == NULL)
    {
        log_error("HashTable: value is NULL");
        return false;
    }
    if (table->header.is_pointer)
    {
        log_error("HashTable: table is a pointer table");
        return false;
    }
    
    for (u64 i = 0; i < table->header.capacity; i++)
    {
        memory_copy(table->data + (table->header.element_size * i), value, table->header.element_size);
    }
    return true;
}