#pragma once

#include <defines.h>

#define BYPASS 2

typedef bool (*TestFunction)(void);

void test_init(void);
void test_register(TestFunction test, const char* name);
void test_run(void);