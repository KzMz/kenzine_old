#include "freelist_tests.h"

#include <lib/memory/freelist.h>
#include "../test.h"
#include "../expect.h"
#include <core/memory.h>

bool freelist_should_create_destroy()
{
    u64 memory_size = freelist_get_nodes_size(1024);
    void* memory = platform_alloc(memory_size, false);

    FreeList list;
    freelist_create(1024, memory, &list);

    expect_eq(list.capacity, 1024 / sizeof(FreeListNode));
    expect_eq(list.total_size, 1024);
    expect_eq(list.head->offset, 0);
    expect_eq(list.head->size, 1024);
    expect_eq(list.head->prev, NULL);
    expect_eq(list.head->next, NULL);
    expect_not_eq(list.nodes, NULL);

    freelist_destroy(&list);
    expect_eq(list.capacity, 0);
    expect_eq(list.total_size, 0);
    expect_eq(list.head, NULL);
    expect_eq(list.nodes, NULL);

    platform_free(memory, false);
    return true;
}

bool freelist_should_alloc_and_free()
{
    u64 memory_size = freelist_get_nodes_size(1024);
    void* memory = platform_alloc(memory_size, false);

    FreeList list;
    freelist_create(1024, memory, &list);

    u64 offset = INVALID_ID;
    bool result = freelist_alloc(&list, 64, &offset);
    expect_true(result);
    expect_eq(offset, 0);

    result = freelist_free(&list, 64, offset);
    expect_true(result);

    freelist_destroy(&list);
    expect_eq(list.capacity, 0);
    expect_eq(list.total_size, 0);
    expect_eq(list.head, NULL);
    expect_eq(list.nodes, NULL);

    platform_free(memory, false);
    return true;
}

bool freelist_should_alloc_and_free_multiple()
{
    u64 memory_size = freelist_get_nodes_size(1024);
    void* memory = platform_alloc(memory_size, false);

    FreeList list;
    freelist_create(1024, memory, &list);

    u64 offset = INVALID_ID;
    bool result = freelist_alloc(&list, 64, &offset);
    expect_true(result);
    expect_eq(offset, 0);

    u64 offset2 = INVALID_ID;
    result = freelist_alloc(&list, 64, &offset2);
    expect_true(result);
    expect_eq(offset2, 64);

    u64 offset3 = INVALID_ID;
    result = freelist_alloc(&list, 64, &offset3);
    expect_true(result);
    expect_eq(offset3, 128);

    result = freelist_free(&list, 64, offset2);
    expect_true(result);

    u64 offset4 = INVALID_ID;
    result = freelist_alloc(&list, 64, &offset4);
    expect_true(result);
    expect_eq(offset4, offset2);

    result = freelist_free(&list, 64, offset);
    expect_true(result);

    result = freelist_free(&list, 64, offset3);
    expect_true(result);

    result = freelist_free(&list, 64, offset4);
    expect_true(result);

    freelist_destroy(&list);
    expect_eq(list.capacity, 0);
    expect_eq(list.total_size, 0);
    expect_eq(list.head, NULL);
    expect_eq(list.nodes, NULL);

    platform_free(memory, false);
    return true;
}

bool freelist_should_alloc_and_free_various()
{
    u64 memory_size = freelist_get_nodes_size(1024);
    void* memory = platform_alloc(memory_size, false);

    FreeList list;
    freelist_create(1024, memory, &list);

    u64 offset = INVALID_ID;
    bool result = freelist_alloc(&list, 64, &offset);
    expect_true(result);
    expect_eq(offset, 0);

    u64 offset2 = INVALID_ID;
    result = freelist_alloc(&list, 128, &offset2);
    expect_true(result);
    expect_eq(offset2, 64);

    u64 offset3 = INVALID_ID;
    result = freelist_alloc(&list, 256, &offset3);
    expect_true(result);
    expect_eq(offset3, 192);

    result = freelist_free(&list, 128, offset2);
    expect_true(result);

    u64 offset4 = INVALID_ID;
    result = freelist_alloc(&list, 128, &offset4);
    expect_true(result);
    expect_eq(offset4, offset2);

    result = freelist_free(&list, 64, offset);
    expect_true(result);

    result = freelist_free(&list, 256, offset3);
    expect_true(result);

    result = freelist_free(&list, 128, offset4);
    expect_true(result);

    freelist_destroy(&list);
    expect_eq(list.capacity, 0);
    expect_eq(list.total_size, 0);
    expect_eq(list.head, NULL);
    expect_eq(list.nodes, NULL);

    platform_free(memory, false);
    return true;
}

bool freelist_should_alloc_full_and_fail()
{
    u64 memory_size = freelist_get_nodes_size(1024);
    void* memory = platform_alloc(memory_size, false);

    FreeList list;
    freelist_create(1024, memory, &list);

    u64 offset = INVALID_ID;
    bool result = freelist_alloc(&list, 1024, &offset);
    expect_true(result);
    expect_eq(offset, 0);

    u64 offset2 = INVALID_ID;
    result = freelist_alloc(&list, 64, &offset2);
    expect_false(result);

    freelist_destroy(&list);
    expect_eq(list.capacity, 0);
    expect_eq(list.total_size, 0);
    expect_eq(list.head, NULL);
    expect_eq(list.nodes, NULL);

    platform_free(memory, false);
    return true;
}

void freelist_register_tests()
{
    test_register(freelist_should_create_destroy, "freelist_should_create_destroy");
    test_register(freelist_should_alloc_and_free, "freelist_should_alloc_and_free");
    test_register(freelist_should_alloc_and_free_multiple, "freelist_should_alloc_and_free_multiple");
    test_register(freelist_should_alloc_and_free_various, "freelist_should_alloc_and_free_various");   
    test_register(freelist_should_alloc_full_and_fail, "freelist_should_alloc_full_and_fail");
}