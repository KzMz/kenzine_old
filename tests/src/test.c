#include "test.h"

#include <lib/containers/dyn_array.h>
#include <core/log.h>
#include <core/clock.h>
#include <lib/string.h>

typedef struct Test
{
    TestFunction function;
    const char* name;
} Test;

static Test* tests;

void test_init(void)
{
    tests = dynarray_create(Test);
}

void test_register(TestFunction test, const char* name)
{
    Test t = { test, name };
    dynarray_push(tests, t);
}

void test_run(void)
{
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 num_tests = dynarray_length(tests);

    Clock total_time;
    clock_start(&total_time);

    for (u32 i = 0; i < num_tests; ++i)
    {
        Clock test_time;
        clock_start(&test_time);

        u8 result = tests[i].function();
        clock_update(&test_time);

        if (result == true) 
        {
            passed++;
        }
        else if (result == BYPASS)
        {
            skipped++;
            log_warning("Test '%s' was skipped", tests[i].name);
        }
        else 
        {
            failed++;
            log_error("Test '%s' failed", tests[i].name);
        }

        char status[32];
        string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
        clock_update(&total_time);
        log_info("Test '%s' %s (%.2f ms)", tests[i].name, status, test_time.elapsed_time);
    }

    clock_stop(&total_time);

    log_info("Tests run: %d passed: %d failed %d skipped %d", num_tests, passed, failed, skipped);
}