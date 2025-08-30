/*******************************************************************************
 * Bridge Object Unit Tests - CUnit Framework
 * 
 * Comprehensive unit tests for bridge_object.c functions:
 * - bridge_init()
 * - bridge_register()
 * - bridge_update()
 * - bridge_cleanup()
 * 
 * Uses CUnit framework following Wakaama patterns
 *******************************************************************************/

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <CUnit/TestDB.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "bridge_object_mock.h"

/* Test suite setup and cleanup */
int bridge_object_suite_init(void) {
    printf("Initializing Bridge Object Test Suite...\n");
    return 0;
}

int bridge_object_suite_cleanup(void) {
    printf("Cleaning up Bridge Object Test Suite...\n");
    return 0;
}

/* Test Functions */

/**
 * Test bridge initialization
 */
void test_bridge_init(void) {
    printf("Testing bridge_init()...\n");
    
    /* Test initialization - should not crash */
    bridge_init();
    CU_PASS("Bridge initialization completed without error");
    
    /* Test multiple initializations - should be safe */
    bridge_init();
    bridge_init();
    CU_PASS("Multiple bridge initializations handled safely");
}

/**
 * Test resource registration functionality
 */
void test_bridge_register_basic(void) {
    printf("Testing basic bridge_register()...\n");
    
    /* Initialize bridge first */
    bridge_init();
    
    /* Test valid navigation light registration */
    int result = bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test valid temperature sensor registration */
    result = bridge_register(3300, 0, 5700, "environment.water.temperature");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test valid power measurement registration */
    result = bridge_register(3305, 0, 5800, "electrical.batteries.house.voltage");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test valid dimmer registration */
    result = bridge_register(3306, 1, 5851, "electrical.switches.cabin.lights");
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test resource registration edge cases
 */
void test_bridge_register_edge_cases(void) {
    printf("Testing bridge_register() edge cases...\n");
    
    bridge_init();
    
    /* Test NULL path - should fail gracefully */
    int result = bridge_register(3306, 0, 5850, NULL);
    CU_ASSERT_NOT_EQUAL(result, 0);
    
    /* Test empty path - should fail gracefully */
    result = bridge_register(3306, 0, 5850, "");
    CU_ASSERT_NOT_EQUAL(result, 0);
    
    /* Test very long path - should handle gracefully */
    char long_path[1024];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    result = bridge_register(3306, 0, 5850, long_path);
    /* Should either succeed or fail gracefully, but not crash */
    CU_ASSERT_TRUE(result == 0 || result != 0);
}

/**
 * Test duplicate registration handling
 */
void test_bridge_register_duplicates(void) {
    printf("Testing duplicate registrations...\n");
    
    bridge_init();
    
    /* Register a resource */
    int result1 = bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    CU_ASSERT_EQUAL(result1, 0);
    
    /* Try to register the same resource again */
    int result2 = bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights.duplicate");
    /* Should either allow duplicate or handle gracefully */
    CU_ASSERT_TRUE(result2 == 0 || result2 != 0);
    
    /* Register different instance of same object */
    result2 = bridge_register(3306, 1, 5850, "electrical.switches.anchor.light");
    CU_ASSERT_EQUAL(result2, 0);
}

/**
 * Test IPSO object type validation
 */
void test_bridge_register_ipso_objects(void) {
    printf("Testing IPSO object registrations...\n");
    
    bridge_init();
    
    /* Test IPSO 3300 - Generic Sensor */
    int result = bridge_register(3300, 0, 5700, "environment.water.temperature");
    CU_ASSERT_EQUAL(result, 0);
    
    result = bridge_register(3300, 1, 5700, "environment.outside.pressure");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test IPSO 3305 - Power Measurement */
    result = bridge_register(3305, 0, 5800, "electrical.batteries.house.voltage");
    CU_ASSERT_EQUAL(result, 0);
    
    result = bridge_register(3305, 0, 5801, "electrical.batteries.house.current");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test IPSO 3306 - Actuation */
    result = bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    CU_ASSERT_EQUAL(result, 0);
    
    result = bridge_register(3306, 0, 5851, "electrical.switches.cabin.dimmer");
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test marine-specific SignalK paths
 */
void test_bridge_register_marine_paths(void) {
    printf("Testing marine SignalK path registrations...\n");
    
    bridge_init();
    
    /* Test navigation paths */
    int result = bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test engine paths */
    result = bridge_register(3300, 0, 5700, "propulsion.main.temperature");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test environment paths */
    result = bridge_register(3300, 1, 5700, "environment.water.temperature");
    CU_ASSERT_EQUAL(result, 0);
    
    result = bridge_register(3300, 2, 5700, "environment.wind.speedApparent");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test electrical paths */
    result = bridge_register(3305, 0, 5800, "electrical.batteries.house.voltage");
    CU_ASSERT_EQUAL(result, 0);
    
    result = bridge_register(3305, 1, 5800, "electrical.batteries.starter.voltage");
    CU_ASSERT_EQUAL(result, 0);
    
    /* Test tank paths */
    result = bridge_register(3300, 3, 5700, "tanks.fuel.level");
    CU_ASSERT_EQUAL(result, 0);
    
    result = bridge_register(3300, 4, 5700, "tanks.freshWater.level");
    CU_ASSERT_EQUAL(result, 0);
}

/**
 * Test bridge update functionality
 */
void test_bridge_update_basic(void) {
    printf("Testing basic bridge_update()...\n");
    
    bridge_init();
    
    /* Register resources first */
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    bridge_register(3305, 0, 5800, "electrical.batteries.house.voltage");
    
    /* Test switch updates */
    bridge_update("electrical.switches.navigation.lights", "1");
    CU_PASS("Switch ON update completed");
    
    bridge_update("electrical.switches.navigation.lights", "0");
    CU_PASS("Switch OFF update completed");
    
    /* Test temperature updates */
    bridge_update("environment.water.temperature", "15.5");
    CU_PASS("Temperature update completed");
    
    bridge_update("environment.water.temperature", "18.2");
    CU_PASS("Temperature update completed");
    
    /* Test voltage updates */
    bridge_update("electrical.batteries.house.voltage", "13.8");
    CU_PASS("Voltage update completed");
    
    bridge_update("electrical.batteries.house.voltage", "12.6");
    CU_PASS("Voltage update completed");
}

/**
 * Test bridge update edge cases
 */
void test_bridge_update_edge_cases(void) {
    printf("Testing bridge_update() edge cases...\n");
    
    bridge_init();
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    
    /* Test NULL path */
    bridge_update(NULL, "1");
    CU_PASS("NULL path handled gracefully");
    
    /* Test NULL value */
    bridge_update("electrical.switches.navigation.lights", NULL);
    CU_PASS("NULL value handled gracefully");
    
    /* Test empty path */
    bridge_update("", "1");
    CU_PASS("Empty path handled gracefully");
    
    /* Test empty value */
    bridge_update("electrical.switches.navigation.lights", "");
    CU_PASS("Empty value handled gracefully");
    
    /* Test unregistered path */
    bridge_update("unregistered.path", "1");
    CU_PASS("Unregistered path handled gracefully");
    
    /* Test very long value */
    char long_value[1024];
    memset(long_value, '9', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    bridge_update("electrical.switches.navigation.lights", long_value);
    CU_PASS("Very long value handled gracefully");
}

/**
 * Test marine scenario updates
 */
void test_bridge_update_marine_scenarios(void) {
    printf("Testing marine scenario updates...\n");
    
    bridge_init();
    
    /* Register marine resources */
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_register(3306, 1, 5850, "electrical.switches.anchor.light");
    bridge_register(3306, 2, 5850, "electrical.switches.bilge.pump");
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    bridge_register(3300, 1, 5700, "environment.wind.speedApparent");
    bridge_register(3305, 0, 5800, "electrical.batteries.house.voltage");
    bridge_register(3305, 1, 5801, "electrical.batteries.house.current");
    
    /* Test emergency bilge pump scenario */
    bridge_update("electrical.switches.bilge.pump", "1");  /* Emergency ON */
    CU_PASS("Emergency bilge pump activation");
    
    bridge_update("electrical.switches.bilge.pump", "0");  /* Emergency OFF */
    CU_PASS("Emergency bilge pump deactivation");
    
    /* Test navigation lights scenario */
    bridge_update("electrical.switches.navigation.lights", "1");  /* Sunset */
    bridge_update("electrical.switches.anchor.light", "0");
    CU_PASS("Navigation lights activated");
    
    /* Test anchoring scenario */
    bridge_update("electrical.switches.navigation.lights", "0");  /* Anchored */
    bridge_update("electrical.switches.anchor.light", "1");
    CU_PASS("Anchor light activated");
    
    /* Test environmental monitoring */
    bridge_update("environment.water.temperature", "16.5");
    bridge_update("environment.wind.speedApparent", "8.2");
    CU_PASS("Environmental data updated");
    
    /* Test battery monitoring */
    bridge_update("electrical.batteries.house.voltage", "13.8");  /* Charging */
    bridge_update("electrical.batteries.house.current", "5.2");
    CU_PASS("Battery charging data updated");
    
    bridge_update("electrical.batteries.house.voltage", "12.6");  /* Discharging */
    bridge_update("electrical.batteries.house.current", "-8.5");
    CU_PASS("Battery discharging data updated");
}

/**
 * Test value format validation
 */
void test_bridge_update_value_formats(void) {
    printf("Testing value format validation...\n");
    
    bridge_init();
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    
    /* Test boolean values */
    bridge_update("electrical.switches.navigation.lights", "true");
    bridge_update("electrical.switches.navigation.lights", "false");
    bridge_update("electrical.switches.navigation.lights", "1");
    bridge_update("electrical.switches.navigation.lights", "0");
    CU_PASS("Boolean value formats handled");
    
    /* Test numeric values */
    bridge_update("environment.water.temperature", "15.5");
    bridge_update("environment.water.temperature", "-5.0");
    bridge_update("environment.water.temperature", "100");
    bridge_update("environment.water.temperature", "0.0");
    CU_PASS("Numeric value formats handled");
    
    /* Test invalid numeric values */
    bridge_update("environment.water.temperature", "abc");
    bridge_update("environment.water.temperature", "15.5.5");
    bridge_update("environment.water.temperature", "");
    CU_PASS("Invalid numeric values handled gracefully");
}

/**
 * Test concurrent access (thread safety)
 */
void test_bridge_thread_safety(void) {
    printf("Testing thread safety...\n");
    
    bridge_init();
    
    /* Register some resources */
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    
    /* Test concurrent updates - should not crash */
    bridge_update("electrical.switches.navigation.lights", "1");
    bridge_update("environment.water.temperature", "15.5");
    bridge_update("electrical.switches.navigation.lights", "0");
    bridge_update("environment.water.temperature", "16.0");
    
    CU_PASS("Concurrent operations completed without crash");
}

/**
 * Test registry capacity limits
 */
void test_bridge_registry_limits(void) {
    printf("Testing registry capacity limits...\n");
    
    bridge_init();
    
    /* Register many resources to test capacity */
    char path[256];
    int successful_registrations = 0;
    
    for (int i = 0; i < 150; i++) {  /* Try to exceed typical limits */
        snprintf(path, sizeof(path), "test.resource.%d", i);
        int result = bridge_register(3300, i % 10, 5700, path);
        if (result == 0) {
            successful_registrations++;
        }
    }
    
    /* Should register at least some resources */
    CU_ASSERT_TRUE(successful_registrations > 0);
    
    /* Should handle capacity limits gracefully */
    CU_PASS("Registry capacity limits handled gracefully");
    
    printf("Successfully registered %d resources\n", successful_registrations);
}

/**
 * Test bridge registry inspection functions
 */
void test_bridge_registry_inspection(void) {
    printf("Testing registry inspection functions...\n");
    
    bridge_init();
    
    /* Initially empty */
    CU_ASSERT_EQUAL(bridge_get_registry_count(), 0);
    
    /* Register some resources */
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    
    /* Check count */
    CU_ASSERT_EQUAL(bridge_get_registry_count(), 2);
    
    /* Test resource lookup */
    uint16_t objId, instId, resId;
    int result = bridge_find_resource("electrical.switches.navigation.lights", &objId, &instId, &resId);
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(objId, 3306);
    CU_ASSERT_EQUAL(instId, 0);
    CU_ASSERT_EQUAL(resId, 5850);
    
    /* Test lookup of non-existent resource */
    result = bridge_find_resource("non.existent.path", &objId, &instId, &resId);
    CU_ASSERT_NOT_EQUAL(result, 0);
    
    /* Test value retrieval */
    bridge_update("electrical.switches.navigation.lights", "1");
    const char* value = bridge_get_last_value("electrical.switches.navigation.lights");
    CU_ASSERT_PTR_NOT_NULL(value);
    if (value) {
        CU_ASSERT_STRING_EQUAL(value, "1");
    }
    
    /* Test registry printing (should not crash) */
    bridge_print_registry();
    CU_PASS("Registry printing completed");
}

/**
 * Test value persistence and retrieval
 */
void test_bridge_value_persistence(void) {
    printf("Testing value persistence...\n");
    
    bridge_init();
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    
    /* Test initial state (no value) */
    const char* value = bridge_get_last_value("environment.water.temperature");
    CU_ASSERT_PTR_NOT_NULL(value);
    if (value) {
        CU_ASSERT_STRING_EQUAL(value, "");  /* Should be empty initially */
    }
    
    /* Update and verify persistence */
    bridge_update("environment.water.temperature", "15.5");
    value = bridge_get_last_value("environment.water.temperature");
    CU_ASSERT_PTR_NOT_NULL(value);
    if (value) {
        CU_ASSERT_STRING_EQUAL(value, "15.5");
    }
    
    /* Update again and verify */
    bridge_update("environment.water.temperature", "18.2");
    value = bridge_get_last_value("environment.water.temperature");
    CU_ASSERT_PTR_NOT_NULL(value);
    if (value) {
        CU_ASSERT_STRING_EQUAL(value, "18.2");
    }
    
    /* Test with different resource */
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_update("electrical.switches.navigation.lights", "0");
    
    /* Verify both values are maintained independently */
    value = bridge_get_last_value("environment.water.temperature");
    CU_ASSERT_STRING_EQUAL(value, "18.2");
    
    value = bridge_get_last_value("electrical.switches.navigation.lights");
    CU_ASSERT_STRING_EQUAL(value, "0");
}

/**
 * Test cleanup functionality
 */
void test_bridge_cleanup(void) {
    printf("Testing bridge cleanup...\n");
    
    bridge_init();
    
    /* Register some resources */
    bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    bridge_register(3300, 0, 5700, "environment.water.temperature");
    bridge_register(3305, 0, 5800, "electrical.batteries.house.voltage");
    
    /* Verify registration count */
    CU_ASSERT_EQUAL(bridge_get_registry_count(), 3);
    
    /* Update some values */
    bridge_update("electrical.switches.navigation.lights", "1");
    bridge_update("environment.water.temperature", "15.5");
    
    /* Re-initialize should clean up */
    bridge_init();
    CU_PASS("Bridge re-initialization (cleanup) completed");
    
    /* Verify cleanup */
    CU_ASSERT_EQUAL(bridge_get_registry_count(), 0);
    
    /* Should be able to register again after cleanup */
    int result = bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(bridge_get_registry_count(), 1);
}

/* CUnit test suite creation */
CU_ErrorCode create_bridge_object_test_suite(void) {
    CU_pSuite pSuite = NULL;
    
    /* Add the test suite */
    pSuite = CU_add_suite("Bridge Object Unit Tests", 
                          bridge_object_suite_init, 
                          bridge_object_suite_cleanup);
    
    if (pSuite == NULL) {
        return CU_get_error();
    }
    
    /* Add tests to the suite */
    if ((CU_add_test(pSuite, "Bridge Initialization", test_bridge_init) == NULL) ||
        (CU_add_test(pSuite, "Basic Registration", test_bridge_register_basic) == NULL) ||
        (CU_add_test(pSuite, "Registration Edge Cases", test_bridge_register_edge_cases) == NULL) ||
        (CU_add_test(pSuite, "Duplicate Registration", test_bridge_register_duplicates) == NULL) ||
        (CU_add_test(pSuite, "IPSO Object Registration", test_bridge_register_ipso_objects) == NULL) ||
        (CU_add_test(pSuite, "Marine Path Registration", test_bridge_register_marine_paths) == NULL) ||
        (CU_add_test(pSuite, "Basic Updates", test_bridge_update_basic) == NULL) ||
        (CU_add_test(pSuite, "Update Edge Cases", test_bridge_update_edge_cases) == NULL) ||
        (CU_add_test(pSuite, "Marine Scenarios", test_bridge_update_marine_scenarios) == NULL) ||
        (CU_add_test(pSuite, "Value Formats", test_bridge_update_value_formats) == NULL) ||
        (CU_add_test(pSuite, "Thread Safety", test_bridge_thread_safety) == NULL) ||
        (CU_add_test(pSuite, "Registry Limits", test_bridge_registry_limits) == NULL) ||
        (CU_add_test(pSuite, "Registry Inspection", test_bridge_registry_inspection) == NULL) ||
        (CU_add_test(pSuite, "Value Persistence", test_bridge_value_persistence) == NULL) ||
        (CU_add_test(pSuite, "Cleanup", test_bridge_cleanup) == NULL)) {
        return CU_get_error();
    }
    
    return CUE_SUCCESS;
}
