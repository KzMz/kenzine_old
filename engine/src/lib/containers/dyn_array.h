#pragma once

#include "defines.h"

#define DYNARRAY_INITIAL_CAPACITY 1
#define DYNARRAY_GROWTH_FACTOR 2

typedef struct DynArrayHeader 
{
    u64 capacity;
    u64 length;
    u64 element_size;
} DynArrayHeader;

typedef struct DynArray 
{
    DynArrayHeader header;
    void* elements;
} DynArray;

KENZINE_API void* _dynarray_create(u64 length, u64 element_size);
KENZINE_API void _dynarray_destroy(void* array);

KENZINE_API DynArrayHeader* _dynarray_header(void* array);

KENZINE_API void* _dynarray_resize(void* array);

KENZINE_API void* _dynarray_push(void* array, const void* element);
KENZINE_API void _dynarray_pop(void* array, void* dest);

KENZINE_API void* _dynarray_insert(void* array, u64 index, const void* element);
KENZINE_API void* _dynarray_remove(void* array, u64 index, void* dest);

#define dynarray_create(type) _dynarray_create(DYNARRAY_INITIAL_CAPACITY, sizeof(type))

#define dynarray_reserve(type, capacity) _dynarray_create(capacity, sizeof(type))

#define dynarray_destroy(array) _dynarray_destroy(array)

#define dynarray_push(array, element)                 \
    do {                                              \
        typeof(element) _element = element;           \
        array = _dynarray_push(array, &_element);     \
    } while (0)

#define dynarray_pop(array, dest) _dynarray_pop(array, dest)

#define dynarray_insert(array, index, element)               \
    do {                                                     \
        typeof(element) _element = element;                  \
        array = _dynarray_insert(array, index, &_element);   \
    } while (0)

#define dynarray_remove(array, index, dest) _dynarray_remove(array, index, dest)

#define dynarray_header(array) _dynarray_header(array)

#define dynarray_clear(array) ((dynarray_header(array))->length = 0)

#define dynarray_length(array) ((dynarray_header(array))->length)
#define dynarray_capacity(array) ((dynarray_header(array))->capacity)
#define dynarray_element_size(array) ((dynarray_header(array))->element_size)

#define dynarray_set_length(array, new_length) ((dynarray_header(array))->length = (new_length))
#define dynarray_set_capacity(array, new_capacity) ((dynarray_header(array))->capacity = (new_capacity))
#define dynarray_set_element_size(array, new_element_size) ((dynarray_header(array))->element_size = (new_element_size))

#define dynarray_empty(array) (dynarray_length(array) == 0)