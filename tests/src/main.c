#include "test.h"
#include <core/log.h>
#include <core/memory.h>    
#include "lib/memory_tests.h"
#include "lib/containers/hashtable_tests.h"
#include "lib/freelist_tests.h"

int main(void)
{
    MemorySystemConfiguration config = { 0 };
    config.allocation_type = MEMORY_ALLOCATION_TYPE_ARENA;
    config.arena_region_size = ARENA_REGION_SIZE;
    memory_init(config);

    test_init();
    log_debug("Running tests...");

    arena_register_tests();
    hashtable_register_tests();
    freelist_register_tests();

    test_run();
    memory_shutdown();
    return 0;
}