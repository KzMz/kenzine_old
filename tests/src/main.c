#include "test.h"
#include <core/log.h>
#include "lib/memory_tests.h"
#include "lib/containers/hashtable_tests.h"
#include "lib/freelist_tests.h"

int main(void)
{
    test_init();
    log_debug("Running tests...");

    arena_register_tests();
    hashtable_register_tests();
    freelist_register_tests();

    test_run();
    return 0;
}