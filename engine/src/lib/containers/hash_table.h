#pragma once

#include "defines.h"

typedef struct HashTableHeader
{
    u64 capacity;
    u64 element_size;
    bool is_pointer; // if not, the data stores a copy of the value. If it is, the ptr needs to be managed by the user
} HashTableHeader;

typedef struct HashTable
{
    HashTableHeader header;
    void* data;
} HashTable;

KENZINE_API void _hashtable_create(u64 capacity, u64 element_size, bool is_pointer, HashTable* out_table);
KENZINE_API void _hashtable_destroy(HashTable* table);

KENZINE_API bool _hashtable_set_value(HashTable* table, const char* key, void* value);
KENZINE_API bool _hashtable_set_pointer(HashTable* table, const char* key, void** value);

KENZINE_API bool _hashtable_get_value(HashTable* table, const char* key, void* out_value);
KENZINE_API bool _hashtable_get_pointer(HashTable* table, const char* key, void** out_value);

KENZINE_API bool _hashtable_fill_with_value(HashTable* table, void* value);

#define hashtable_create(type, capacity, is_pointer, out_table) _hashtable_create((capacity), sizeof(type), is_pointer, (out_table))

#define hashtable_destroy(table) _hashtable_destroy(table)

#define hashtable_set(table, key, value)                                 \
    do                                                                   \
    {                                                                    \
        if ((table)->header.is_pointer)                                  \
        {                                                                \
            _hashtable_set_pointer((table), (key), (void**)(value));     \
        }                                                                \
        else                                                             \
        {                                                                \
            _hashtable_set_value((table), (key), (void*)(value));        \
        }                                                                \
    } while (0)

#define hashtable_get(table, key, out_value)                             \
    do                                                                   \
    {                                                                    \
        if ((table)->header.is_pointer)                                  \
        {                                                                \
            _hashtable_get_pointer((table), (key), (void**)(out_value)); \
        }                                                                \
        else                                                             \
        {                                                                \
            _hashtable_get_value((table), (key), (void*)(out_value));    \
        }                                                                \
    } while (0)

#define hashtable_fill_with_value(table, value) _hashtable_fill_with_value((table), (void*)(value))