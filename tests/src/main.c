#include "test.h"
#include <core/log.h>
#include "lib/memory_tests.h"

int main(void)
{
    test_init();
    log_debug("Running tests...");

    register_arena_tests();

    test_run();
    return 0;
}