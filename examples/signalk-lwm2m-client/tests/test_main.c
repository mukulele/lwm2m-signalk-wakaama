/**
 * @file test_main.c
 * @brief SignalK-LwM2M Client Test Suite Main Runner
 * 
 * Professional test runner following Wakaama standards using CUnit.
 * Provides comprehensive testing for marine IoT SignalK-LwM2M integration.
 * 
 * @author SignalK-LwM2M Integration Team
 * @date 2025
 */

#include "test_framework.h"
#include <getopt.h>
#include <sys/time.h>

/* ============================================================================
 * Global Test Configuration
 * ============================================================================ */

static test_config_t g_test_config = {
    .suite = TEST_SUITE_ALL,
    .verbose = false,
    .xml_output = false,
    .output_file = NULL,
    .timeout_seconds = 30
};

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get current time in milliseconds
 */
static double get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

/**
 * @brief Print program usage
 */
static void print_usage(const char* program_name)
{
    printf("SignalK-LwM2M Client Test Suite\n");
    printf("Professional testing framework following Wakaama standards\n\n");
    
    printf("USAGE:\n");
    printf("    %s [OPTIONS]\n\n", program_name);
    
    printf("OPTIONS:\n");
    printf("    -s, --suite SUITE     Test suite to run (all|connection|bridge|sensors|reconnection|config)\n");
    printf("    -v, --verbose         Enable verbose output\n");
    printf("    -x, --xml             Generate XML output (JUnit format)\n");
    printf("    -o, --output FILE     Output file for XML results\n");
    printf("    -t, --timeout SEC     Test timeout in seconds (default: 30)\n");
    printf("    -h, --help            Show this help message\n\n");
    
    printf("EXAMPLES:\n");
    printf("    %s                              # Run all tests\n", program_name);
    printf("    %s --suite connection --verbose # Run connection tests with verbose output\n", program_name);
    printf("    %s --xml --output results.xml   # Generate XML test report\n", program_name);
    printf("    %s --suite bridge               # Run bridge object tests only\n\n", program_name);
    
    printf("TEST SUITES:\n");
    printf("    all           Complete test suite (default)\n");
    printf("    connection    SignalK WebSocket connection tests\n");
    printf("    bridge        Bridge object functionality tests\n");
    printf("    sensors       Marine sensor integration tests\n");
    printf("    reconnection  Network reconnection handling tests\n");
    printf("    config        Configuration management tests\n\n");
    
    printf("MARINE IoT FEATURES TESTED:\n");
    printf("    🌊 SignalK WebSocket connectivity and authentication\n");
    printf("    ⚓ LwM2M bridge object creation and management\n");
    printf("    📡 Marine sensor data acquisition and processing\n");
    printf("    🔄 Network reconnection and error recovery\n");
    printf("    ⚙️  Configuration hot-reload and validation\n");
    printf("    📊 Performance benchmarking and memory management\n\n");
}

/**
 * @brief Parse test suite from string
 */
static test_suite_t parse_test_suite(const char* suite_str)
{
    if (strcmp(suite_str, "all") == 0) return TEST_SUITE_ALL;
    if (strcmp(suite_str, "connection") == 0) return TEST_SUITE_CONNECTION;
    if (strcmp(suite_str, "bridge") == 0) return TEST_SUITE_BRIDGE;
    if (strcmp(suite_str, "sensors") == 0) return TEST_SUITE_SENSORS;
    if (strcmp(suite_str, "reconnection") == 0) return TEST_SUITE_RECONNECTION;
    if (strcmp(suite_str, "config") == 0) return TEST_SUITE_CONFIGURATION;
    
    fprintf(stderr, "Error: Unknown test suite '%s'\n", suite_str);
    fprintf(stderr, "Valid suites: all, connection, bridge, sensors, reconnection, config\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Parse command line arguments
 */
static void parse_arguments(int argc, char* argv[])
{
    static struct option long_options[] = {
        {"suite",    required_argument, 0, 's'},
        {"verbose",  no_argument,       0, 'v'},
        {"xml",      no_argument,       0, 'x'},
        {"output",   required_argument, 0, 'o'},
        {"timeout",  required_argument, 0, 't'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "s:vxo:t:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 's':
                g_test_config.suite = parse_test_suite(optarg);
                break;
            case 'v':
                g_test_config.verbose = true;
                break;
            case 'x':
                g_test_config.xml_output = true;
                break;
            case 'o':
                g_test_config.output_file = optarg;
                break;
            case 't':
                g_test_config.timeout_seconds = atoi(optarg);
                if (g_test_config.timeout_seconds <= 0) {
                    fprintf(stderr, "Error: Invalid timeout value '%s'\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Error: Unknown option\n");
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }
    
    /* Check for conflicting options */
    if (g_test_config.xml_output && !g_test_config.output_file) {
        g_test_config.output_file = "test_results.xml";
        if (g_test_config.verbose) {
            printf("Info: XML output enabled, using default file: %s\n", g_test_config.output_file);
        }
    }
}

/* ============================================================================
 * Test Framework Implementation
 * ============================================================================ */

CU_ErrorCode test_framework_init(const test_config_t* config)
{
    CU_ErrorCode result;
    
    /* Initialize CUnit registry */
    result = CU_initialize_registry();
    if (result != CUE_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize CUnit registry: %s\n", 
                CU_get_error_msg());
        return result;
    }
    
    if (config->verbose) {
        printf("🌊 SignalK-LwM2M Client Test Framework Initialized\n");
        printf("========================================\n");
        printf("Test Suite: %s\n", 
               config->suite == TEST_SUITE_ALL ? "All" :
               config->suite == TEST_SUITE_CONNECTION ? "Connection" :
               config->suite == TEST_SUITE_BRIDGE ? "Bridge" :
               config->suite == TEST_SUITE_SENSORS ? "Sensors" :
               config->suite == TEST_SUITE_RECONNECTION ? "Reconnection" :
               config->suite == TEST_SUITE_CONFIGURATION ? "Configuration" : "Unknown");
        printf("Verbose Mode: Enabled\n");
        printf("XML Output: %s\n", config->xml_output ? "Enabled" : "Disabled");
        if (config->output_file) {
            printf("Output File: %s\n", config->output_file);
        }
        printf("Timeout: %d seconds\n", config->timeout_seconds);
        printf("========================================\n\n");
    }
    
    return CUE_SUCCESS;
}

void test_framework_cleanup(void)
{
    CU_cleanup_registry();
}

CU_ErrorCode test_framework_run_suite(test_suite_t suite, test_results_t* results)
{
    CU_ErrorCode reg_result = CUE_SUCCESS;
    double start_time = get_time_ms();
    
    /* Register test suites based on selection */
    switch (suite) {
        case TEST_SUITE_ALL:
            reg_result |= register_signalk_connection_tests();
            reg_result |= register_bridge_object_tests();
            reg_result |= register_marine_sensor_tests();
            reg_result |= register_reconnection_tests();
            reg_result |= register_configuration_tests();
            break;
            
        case TEST_SUITE_CONNECTION:
            reg_result = register_signalk_connection_tests();
            break;
            
        case TEST_SUITE_BRIDGE:
            reg_result = register_bridge_object_tests();
            break;
            
        case TEST_SUITE_SENSORS:
            reg_result = register_marine_sensor_tests();
            break;
            
        case TEST_SUITE_RECONNECTION:
            reg_result = register_reconnection_tests();
            break;
            
        case TEST_SUITE_CONFIGURATION:
            reg_result = register_configuration_tests();
            break;
    }
    
    if (reg_result != CUE_SUCCESS) {
        fprintf(stderr, "Error: Failed to register test suites: %s\n", CU_get_error_msg());
        return reg_result;
    }
    
    /* Setup test environment */
    test_setup_mock_environment();
    
    /* Run tests based on output mode */
    CU_ErrorCode run_result;
    if (g_test_config.xml_output) {
        CU_set_output_filename(g_test_config.output_file);
        run_result = CU_automated_run_tests();
    } else if (g_test_config.verbose) {
        CU_basic_set_mode(CU_BRM_VERBOSE);
        run_result = CU_basic_run_tests();
    } else {
        CU_basic_set_mode(CU_BRM_NORMAL);
        run_result = CU_basic_run_tests();
    }
    
    /* Collect results */
    if (results) {
        results->total_tests = CU_get_number_of_tests_run();
        results->failed_tests = CU_get_number_of_failures();
        results->passed_tests = results->total_tests - results->failed_tests;
        results->skipped_tests = 0; /* CUnit doesn't track skipped tests separately */
        results->execution_time = get_time_ms() - start_time;
    }
    
    /* Cleanup test environment */
    test_cleanup_mock_environment();
    
    return run_result;
}

void test_framework_print_results(const test_results_t* results)
{
    printf("\n========================================\n");
    printf("🌊 SignalK-LwM2M Test Results Summary\n");
    printf("========================================\n");
    printf("Total Tests:    %d\n", results->total_tests);
    printf("Passed:         %d\n", results->passed_tests);
    printf("Failed:         %d\n", results->failed_tests);
    printf("Success Rate:   %.1f%%\n", 
           results->total_tests > 0 ? 
           (results->passed_tests * 100.0 / results->total_tests) : 0.0);
    printf("Execution Time: %.2f ms\n", results->execution_time);
    
    if (results->failed_tests == 0) {
        printf("\n✅ All tests passed! Your SignalK-LwM2M client is ready for marine deployment.\n");
        printf("🌊 Ready for sea trials! ⚓\n");
    } else {
        printf("\n❌ %d test(s) failed. Please review the output above.\n", results->failed_tests);
        printf("🔧 Your marine IoT system needs attention before deployment.\n");
    }
    printf("========================================\n");
}

/* ============================================================================
 * Mock Environment Setup (placeholder implementations)
 * ============================================================================ */

void test_setup_mock_environment(void)
{
    if (g_test_config.verbose) {
        printf("Setting up mock environment for marine IoT testing...\n");
    }
    
    /* TODO: Setup mock SignalK server */
    /* TODO: Setup mock marine sensors */
    /* TODO: Setup mock network conditions */
    /* TODO: Initialize memory leak detection */
    
    test_memory_leak_detection_start();
}

void test_cleanup_mock_environment(void)
{
    if (g_test_config.verbose) {
        printf("Cleaning up mock environment...\n");
    }
    
    /* Check for memory leaks */
    size_t leaked_bytes = test_memory_leak_detection_check();
    if (leaked_bytes > 0) {
        fprintf(stderr, "Warning: Memory leak detected: %zu bytes\n", leaked_bytes);
    }
    
    /* TODO: Cleanup mock SignalK server */
    /* TODO: Cleanup mock marine sensors */
    /* TODO: Cleanup mock network conditions */
}

/* ============================================================================
 * Main Function
 * ============================================================================ */

int main(int argc, char* argv[])
{
    /* Parse command line arguments */
    parse_arguments(argc, argv);
    
    /* Print banner */
    if (!g_test_config.xml_output) {
        printf("\n🌊 SignalK-LwM2M Client Test Suite\n");
        printf("Professional marine IoT testing framework\n");
        printf("Following Wakaama standards with CUnit\n\n");
    }
    
    /* Initialize test framework */
    CU_ErrorCode init_result = test_framework_init(&g_test_config);
    if (init_result != CUE_SUCCESS) {
        fprintf(stderr, "Failed to initialize test framework\n");
        return EXIT_FAILURE;
    }
    
    /* Run tests */
    test_results_t results = {0};
    CU_ErrorCode run_result = test_framework_run_suite(g_test_config.suite, &results);
    
    /* Print results summary (if not XML mode) */
    if (!g_test_config.xml_output) {
        test_framework_print_results(&results);
    }
    
    /* Cleanup */
    test_framework_cleanup();
    
    /* Return appropriate exit code */
    if (run_result != CUE_SUCCESS || results.failed_tests > 0) {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
