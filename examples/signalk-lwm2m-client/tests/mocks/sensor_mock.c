/**
 * @file mocks/sensor_mock.c
 * @brief Sensor Mock Implementation
 */

#include "../marine_test_mocks.h"

/* Test utility function implementations */
void* test_malloc(size_t size)
{
    return malloc(size);
}

void test_free(void* ptr)
{
    if (ptr) {
        free(ptr);
    }
}
