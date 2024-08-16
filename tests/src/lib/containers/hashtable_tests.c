#include "hashtable_tests.h"
#include "../../expect.h"
#include "../../test.h"
#include <lib/containers/hash_table.h>

bool hashtable_should_create_and_destroy(void)
{
    HashTable table = {0}; 
    hashtable_create(u64, 3, false, &table);

    expect_eq(table.header.capacity, 3);
    expect_eq(table.header.element_size, sizeof(u64));
    expect_eq(table.header.is_pointer, false);
    expect_not_eq(table.data, NULL);

    hashtable_destroy(&table);

    expect_eq(table.header.capacity, 0);
    expect_eq(table.header.element_size, 0);
    expect_eq(table.header.is_pointer, false);
    expect_eq(table.data, NULL);

    return true;
}

bool hashtable_should_get_and_set_value(void)
{
    HashTable table = {0};
    hashtable_create(u64, 3, false, &table);

    expect_eq(table.header.capacity, 3);
    expect_eq(table.header.element_size, sizeof(u64));
    expect_eq(table.header.is_pointer, false);
    expect_not_eq(table.data, NULL);

    u64 value = 52;
    hashtable_set(&table, "test1", &value);
    u64 out_value = 0;
    hashtable_get(&table, "test1", &out_value);
    expect_eq(value, out_value);

    hashtable_destroy(&table);

    expect_eq(table.header.capacity, 0);
    expect_eq(table.header.element_size, 0);
    expect_eq(table.header.is_pointer, false);
    expect_eq(table.data, NULL);

    return true;
}

typedef struct TestStruct
{
    bool bvalue;
    u64 uvalue;
    f32 fvalue;
} TestStruct;

bool hashtable_should_get_and_set_pointer(void)
{
    HashTable table = {0};
    hashtable_create(TestStruct*, 3, true, &table);

    expect_eq(table.header.capacity, 3);
    expect_eq(table.header.element_size, sizeof(TestStruct*));
    expect_eq(table.header.is_pointer, true);
    expect_not_eq(table.data, NULL);

    TestStruct test = {true, 42, 3.14f};
    TestStruct* ptr = &test;
    hashtable_set(&table, "test1", &ptr);

    TestStruct* out_ptr = NULL;
    hashtable_get(&table, "test1", &out_ptr);

    expect_eq(ptr->bvalue, out_ptr->bvalue);
    expect_eq(ptr->uvalue, out_ptr->uvalue);
    expect_eq_f(ptr->fvalue, out_ptr->fvalue);

    hashtable_destroy(&table);

    expect_eq(table.header.capacity, 0);
    expect_eq(table.header.element_size, 0);
    expect_eq(table.header.is_pointer, false);
    expect_eq(table.data, NULL);

    return true;
}

bool hashtable_should_get_and_set_value_nonexist(void)
{
    HashTable table = {0};
    hashtable_create(u64, 3, false, &table);

    expect_eq(table.header.capacity, 3);
    expect_eq(table.header.element_size, sizeof(u64));
    expect_eq(table.header.is_pointer, false);
    expect_not_eq(table.data, NULL);

    u64 value = 52;
    hashtable_set(&table, "test1", &value);
    u64 out_value = 0;
    hashtable_get(&table, "test2", &out_value);
    expect_eq(0, out_value);

    hashtable_destroy(&table);

    expect_eq(table.header.capacity, 0);
    expect_eq(table.header.element_size, 0);
    expect_eq(table.header.is_pointer, false);
    expect_eq(table.data, NULL);

    return true;
}

bool hashtable_should_get_and_set_pointer_nonexist(void)
{
    HashTable table = {0};
    hashtable_create(TestStruct*, 3, true, &table);

    expect_eq(table.header.capacity, 3);
    expect_eq(table.header.element_size, sizeof(TestStruct*));
    expect_eq(table.header.is_pointer, true);
    expect_not_eq(table.data, NULL);

    TestStruct test = {true, 42, 3.14f};
    TestStruct* ptr = &test;
    hashtable_set(&table, "test1", &ptr);

    TestStruct* out_ptr = NULL;
    hashtable_get(&table, "test2", &out_ptr);
    expect_eq(NULL, out_ptr);

    hashtable_destroy(&table);

    expect_eq(table.header.capacity, 0);
    expect_eq(table.header.element_size, 0);
    expect_eq(table.header.is_pointer, false);
    expect_eq(table.data, NULL);

    return true;
}

bool hashtable_should_set_and_update_pointer(void)
{
    HashTable table = {0};
    hashtable_create(TestStruct*, 3, true, &table);

    expect_eq(table.header.capacity, 3);
    expect_eq(table.header.element_size, sizeof(TestStruct*));
    expect_eq(table.header.is_pointer, true);
    expect_not_eq(table.data, NULL);

    TestStruct test = {true, 52, 3.14f};
    TestStruct* ptr = &test;
    hashtable_set(&table, "test1", &ptr);

    TestStruct* out_ptr = NULL;
    hashtable_get(&table, "test1", &out_ptr);

    expect_eq(ptr->bvalue, out_ptr->bvalue);
    expect_eq(ptr->uvalue, out_ptr->uvalue);
    expect_eq_f(ptr->fvalue, out_ptr->fvalue);

    out_ptr->bvalue = false;
    out_ptr->uvalue = 21;
    out_ptr->fvalue = 2.71f;

    TestStruct* out_ptr2 = NULL;
    hashtable_get(&table, "test1", &out_ptr2);
    expect_false(out_ptr2->bvalue);
    expect_eq(21, out_ptr2->uvalue);
    expect_eq_f(2.71f, out_ptr2->fvalue);

    hashtable_destroy(&table);

    expect_eq(table.header.capacity, 0);
    expect_eq(table.header.element_size, 0);
    expect_eq(table.header.is_pointer, false);
    expect_eq(table.data, NULL);

    return true;
}

void hashtable_register_tests(void)
{
    test_register(hashtable_should_create_and_destroy, "hashtable_should_create_and_destroy");
    test_register(hashtable_should_get_and_set_value, "hashtable_should_get_and_set_value");
    test_register(hashtable_should_get_and_set_pointer, "hashtable_should_get_and_set_pointer");
    test_register(hashtable_should_get_and_set_value_nonexist, "hashtable_should_get_and_set_value_nonexist");
    test_register(hashtable_should_get_and_set_pointer_nonexist, "hashtable_should_get_and_set_pointer_nonexist");
    test_register(hashtable_should_set_and_update_pointer, "hashtable_should_set_and_update_pointer");
}