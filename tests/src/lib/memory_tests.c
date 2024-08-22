#include "memory_tests.h"
#include <lib/memory/arena.h> 
#include "../expect.h"
#include "../test.h"

bool test_arena_alloc_clear(void)
{
    Arena arena = {0};
    u8* alloc = arena_alloc(&arena, sizeof(u8) * 10, false);
    expect_not_eq(alloc, NULL);
    expect_eq(arena.num_allocations, 1);
    expect_eq(arena.num_dynamic_allocations, 1);
    expect_eq(arena_get_size(&arena), sizeof(u8) * 10);
    expect_eq(arena_get_max_size(&arena), REGION_DEFAULT_SIZE);

    arena_clear(&arena);
    return true;
}

bool test_arena_over_default(void)
{
    Arena arena = {0};
    u8* alloc = arena_alloc(&arena, REGION_DEFAULT_SIZE * 2, false);
    expect_not_eq(alloc, NULL);
    expect_eq(arena.num_allocations, 1);
    expect_eq(arena.num_dynamic_allocations, 1);
    expect_eq(arena_get_size(&arena), REGION_DEFAULT_SIZE * 2);
    expect_eq(arena_get_max_size(&arena), REGION_DEFAULT_SIZE * 2);

    u8* alloc2 = arena_alloc(&arena, sizeof(u8) * 10, false);
    expect_not_eq(alloc2, NULL);
    expect_eq(arena.num_allocations, 2);
    expect_eq(arena.num_dynamic_allocations, 2);
    expect_eq(arena_get_size(&arena), REGION_DEFAULT_SIZE * 2 + sizeof(u8) * 10);

    u8* alloc3 = arena_alloc(&arena, sizeof(u8) * 2, false);
    expect_not_eq(alloc3, NULL);
    expect_eq(arena.num_allocations, 3);
    expect_eq(arena.num_dynamic_allocations, 2);
    expect_eq(arena_get_size(&arena), REGION_DEFAULT_SIZE * 2 + sizeof(u8) * 10 + sizeof(u8) * 2);

    arena_clear(&arena);
    return true;
}

void arena_register_tests(void)
{
    test_register(test_arena_alloc_clear, "arena_alloc_clear");
    test_register(test_arena_over_default, "arena_over_default");
}