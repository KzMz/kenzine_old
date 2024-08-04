#include "dyn_array.h"
#include "core/memory.h"

KENZINE_API void* _dynarray_create(u64 length, u64 element_size)
{
    const u64 header_size = sizeof(DynArrayHeader);
    const u64 array_size = length * element_size;
    const u64 total_size = header_size + array_size;

    void* new_array = memory_alloc(total_size, MEMORY_TAG_DYNARRAY);
    memory_zero(new_array, total_size);

    DynArrayHeader* header = (DynArrayHeader*) new_array;
    header->capacity = length;
    header->length = 0;
    header->element_size = element_size;

    return (new_array + sizeof(DynArrayHeader));
}

KENZINE_API void _dynarray_destroy(void* array)
{
    DynArrayHeader* header = dynarray_header(array);
    const u64 header_size = sizeof(DynArrayHeader);
    const u64 array_size = header->capacity * header->element_size;
    const u64 total_size = header_size + array_size;

    memory_free(header, total_size, MEMORY_TAG_DYNARRAY);
}

KENZINE_API DynArrayHeader* _dynarray_header(void* array)
{
    return (DynArrayHeader*) (array - sizeof(DynArrayHeader));
}

KENZINE_API void* _dynarray_resize(void* array)
{
    const u64 length = dynarray_length(array);
    const u64 element_size = dynarray_element_size(array);
    const u64 capacity = dynarray_capacity(array);
    void* new_array = _dynarray_create(capacity * DYNARRAY_GROWTH_FACTOR, element_size);
    memory_copy(new_array, array, length * element_size);

    dynarray_set_length(new_array, length);
    _dynarray_destroy(array);

    return new_array;
}

KENZINE_API void* _dynarray_push(void* array, const void* element_ptr)
{
    const u64 length = dynarray_length(array);
    const u64 element_size = dynarray_element_size(array);
    if (length >= dynarray_capacity(array))
    {
        array = _dynarray_resize(array);
    }

    u64 addr = (u64) array + (length * element_size);
    memory_copy((void*) addr, element_ptr, element_size);
    dynarray_set_length(array, length + 1);

    return array;
}

KENZINE_API void _dynarray_pop(void* array, void* dest)
{
    const u64 length = dynarray_length(array);
    const u64 element_size = dynarray_element_size(array);
    u64 addr = (u64) array + ((length - 1) * element_size);
    memory_copy(dest, (void*) addr, element_size);
    dynarray_set_length(array, length - 1);
}

KENZINE_API void* _dynarray_remove(void* array, u64 index, void* dest)
{
    const u64 length = dynarray_length(array);
    const u64 element_size = dynarray_element_size(array);
    if (index >= length)
    {
        log_error("Index out of bounds. Index: %llu, Length: %llu", index, length);
        return array;
    }

    u64 base_addr = (u64) array;
    memory_copy(dest, (void*) (base_addr + (index * element_size)), element_size);

    // if not the last one, copy the rest
    if (index != length - 1)
    {
        memory_copy((void*) (base_addr + (index * element_size)),
                    (void*) (base_addr + ((index + 1) * element_size)),
                    (length - index) * element_size);
    }

    dynarray_set_length(array, length - 1);
    return array;
}

KENZINE_API void* _dynarray_insert(void* array, u64 index, const void* element_ptr)
{
    const u64 length = dynarray_length(array);
    const u64 element_size = dynarray_element_size(array);
    if (index > length)
    {
        log_error("Index out of bounds. Index: %llu, Length: %llu", index, length);
        return array;
    }

    if (length >= dynarray_capacity(array))
    {
        array = _dynarray_resize(array);
    }

    u64 base_addr = (u64) array;
    // if not the last one, copy the rest
    if (index != length)
    {
        memory_copy((void*) (base_addr + ((index + 1) * element_size)),
                    (void*) (base_addr + (index * element_size)),
                    (length - index) * element_size);
    }

    memory_copy((void*) (base_addr + (index * element_size)), element_ptr, element_size);
    dynarray_set_length(array, length + 1);

    return array;
}