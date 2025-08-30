/**
 * @file test_framework.h
 * @brief SignalK-LwM2M Client Test Framework
 * 
 * Professional test framework following Wakaama standards using CUnit.
 * Provides comprehensive testing infrastructure for marine IoT applications.
 * 
 * @author SignalK-LwM2M Integration Team
 * @date 2025
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>
#include <CUnit/Automated.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

/* Wakaama includes */
#include "liblwm2m.h"

/* Project includes */
#include "signalk_test_utils.h"
#include "marine_test_mocks.h"

/**
 * @brief Test suite definitions
 */
typedef enum {
    TEST_SUITE_CONNECTION,
    TEST_SUITE_BRIDGE,
    TEST_SUITE_SENSORS,
    TEST_SUITE_RECONNECTION,
    TEST_SUITE_CONFIGURATION,
    TEST_SUITE_ALL
} test_suite_t;

/**
 * @brief Test result structure
 */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
    double execution_time;
} test_results_t;

/**
 * @brief Test configuration
 */
typedef struct {
    test_suite_t suite;
    bool verbose;
    bool xml_output;
    const char* output_file;
    int timeout_seconds;
} test_config_t;

/* ============================================================================
 * Test Framework Functions
 * ============================================================================ */

/**
 * @brief Initialize the test framework
 * @param config Test configuration
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode test_framework_init(const test_config_t* config);

/**
 * @brief Cleanup the test framework
 */
void test_framework_cleanup(void);

/**
 * @brief Run specified test suite
 * @param suite Test suite to run
 * @param results Output results structure
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode test_framework_run_suite(test_suite_t suite, test_results_t* results);

/**
 * @brief Print test results summary
 * @param results Test results to display
 */
void test_framework_print_results(const test_results_t* results);

/* ============================================================================
 * Test Suite Registration Functions
 * ============================================================================ */

/**
 * @brief Register SignalK connection test suite
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode register_signalk_connection_tests(void);

/**
 * @brief Register bridge object test suite
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode register_bridge_object_tests(void);

/**
 * @brief Register marine sensor test suite
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode register_marine_sensor_tests(void);

/**
 * @brief Register reconnection test suite
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode register_reconnection_tests(void);

/**
 * @brief Register configuration test suite
 * @return CUE_SUCCESS on success, error code otherwise
 */
CU_ErrorCode register_configuration_tests(void);

/* ============================================================================
 * Test Utilities and Assertions
 * ============================================================================ */

/**
 * @brief Enhanced assertion macros for marine IoT testing
 */
#define CU_ASSERT_SIGNALK_SUCCESS(result) \
    CU_ASSERT_EQUAL(result, SIGNALK_SUCCESS)

#define CU_ASSERT_SIGNALK_ERROR(result) \
    CU_ASSERT_NOT_EQUAL(result, SIGNALK_SUCCESS)

#define CU_ASSERT_LWM2M_SUCCESS(result) \
    CU_ASSERT_EQUAL(result, COAP_NO_ERROR)

#define CU_ASSERT_WEBSOCKET_CONNECTED(ws) \
    CU_ASSERT_TRUE((ws) != NULL && (ws)->state == WS_STATE_CONNECTED)

#define CU_ASSERT_BRIDGE_OBJECT_VALID(obj) \
    CU_ASSERT_TRUE((obj) != NULL && (obj)->objID != LWM2M_MAX_ID)

#define CU_ASSERT_MARINE_SENSOR_DATA_VALID(data) \
    CU_ASSERT_TRUE((data) != NULL && (data)->type != SENSOR_TYPE_INVALID)

/**
 * @brief Timeout handling for async operations
 */
#define TEST_TIMEOUT_DEFAULT 5
#define TEST_TIMEOUT_NETWORK 10
#define TEST_TIMEOUT_RECONNECT 30

/**
 * @brief Test helper for waiting with timeout
 * @param condition Condition to wait for
 * @param timeout_sec Timeout in seconds
 * @return true if condition met, false if timeout
 */
bool test_wait_for_condition(bool (*condition)(void), int timeout_sec);

/**
 * @brief Setup mock environment for testing
 */
void test_setup_mock_environment(void);

/**
 * @brief Cleanup mock environment after testing
 */
void test_cleanup_mock_environment(void);

/* ============================================================================
 * Marine IoT Specific Test Utilities
 * ============================================================================ */

/**
 * @brief Create mock SignalK server for testing
 * @param port Port to bind mock server
 * @return Mock server handle or NULL on error
 */
void* test_create_mock_signalk_server(int port);

/**
 * @brief Destroy mock SignalK server
 * @param server Mock server handle
 */
void test_destroy_mock_signalk_server(void* server);

/**
 * @brief Create test marine sensor data
 * @param sensor_type Type of sensor to simulate
 * @return Test sensor data structure
 */
marine_sensor_data_t* test_create_marine_sensor_data(marine_sensor_type_t sensor_type);

/**
 * @brief Validate LwM2M object structure
 * @param object LwM2M object to validate
 * @param expected_obj_id Expected object ID
 * @return true if valid, false otherwise
 */
bool test_validate_lwm2m_object(lwm2m_object_t* object, uint16_t expected_obj_id);

/**
 * @brief Test network connectivity simulation
 * @param simulate_disconnect If true, simulate network disconnect
 */
void test_simulate_network_conditions(bool simulate_disconnect);

/* ============================================================================
 * Performance and Memory Testing
 * ============================================================================ */

/**
 * @brief Memory leak detection setup
 */
void test_memory_leak_detection_start(void);

/**
 * @brief Memory leak detection check
 * @return Number of leaked bytes (0 if no leaks)
 */
size_t test_memory_leak_detection_check(void);

/**
 * @brief Performance benchmark structure
 */
typedef struct {
    const char* operation_name;
    double start_time;
    double end_time;
    double duration_ms;
    size_t memory_usage;
} test_benchmark_t;

/**
 * @brief Start performance benchmark
 * @param benchmark Benchmark structure to initialize
 * @param operation_name Name of operation being benchmarked
 */
void test_benchmark_start(test_benchmark_t* benchmark, const char* operation_name);

/**
 * @brief End performance benchmark
 * @param benchmark Benchmark structure to finalize
 */
void test_benchmark_end(test_benchmark_t* benchmark);

/**
 * @brief Print benchmark results
 * @param benchmark Benchmark results to display
 */
void test_benchmark_print(const test_benchmark_t* benchmark);

/* ============================================================================
 * Constants and Configuration
 * ============================================================================ */

/* Test configuration constants */
#define TEST_SIGNALK_SERVER_URL "ws://localhost:3000/signalk/v1/stream"
#define TEST_SIGNALK_AUTH_TOKEN "test_token_123456789"
#define TEST_LWM2M_SERVER_PORT 5683
#define TEST_CLIENT_ENDPOINT_NAME "test_signalk_client"

/* Marine sensor object IDs (following IPSO standards) */
#define TEST_OBJ_ID_TEMPERATURE 3303
#define TEST_OBJ_ID_HUMIDITY 3304
#define TEST_OBJ_ID_PRESSURE 3323
#define TEST_OBJ_ID_GPS_LOCATION 3336
#define TEST_OBJ_ID_MARINE_BRIDGE 32000  /* Custom marine bridge object */

/* Test data limits */
#define TEST_MAX_BUFFER_SIZE 4096
#define TEST_MAX_SENSORS 32
#define TEST_MAX_BRIDGE_OBJECTS 16

#endif /* TEST_FRAMEWORK_H */
