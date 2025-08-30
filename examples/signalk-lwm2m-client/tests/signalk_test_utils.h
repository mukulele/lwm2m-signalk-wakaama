/**
 * @file signalk_test_utils.h
 * @brief SignalK Test Utilities
 * 
 * Common utilities and helpers for SignalK-LwM2M testing following
 * Wakaama standards and marine IoT best practices.
 * 
 * @author SignalK-LwM2M Integration Team
 * @date 2025
 */

#ifndef SIGNALK_TEST_UTILS_H
#define SIGNALK_TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

/* ============================================================================
 * SignalK Test Constants
 * ============================================================================ */

/* SignalK return codes */
typedef enum {
    SIGNALK_SUCCESS = 0,
    SIGNALK_ERROR_CONNECTION = -1,
    SIGNALK_ERROR_AUTH = -2,
    SIGNALK_ERROR_TIMEOUT = -3,
    SIGNALK_ERROR_PARSE = -4,
    SIGNALK_ERROR_INVALID_DATA = -5
} signalk_result_t;

/* WebSocket states */
typedef enum {
    WS_STATE_DISCONNECTED = 0,
    WS_STATE_CONNECTING,
    WS_STATE_CONNECTED,
    WS_STATE_ERROR
} websocket_state_t;

/* Marine sensor types */
typedef enum {
    SENSOR_TYPE_INVALID = 0,
    SENSOR_TYPE_GPS,
    SENSOR_TYPE_COMPASS,
    SENSOR_TYPE_WIND,
    SENSOR_TYPE_DEPTH,
    SENSOR_TYPE_TEMPERATURE,
    SENSOR_TYPE_PRESSURE,
    SENSOR_TYPE_HUMIDITY
} marine_sensor_type_t;

/* ============================================================================
 * Test Data Structures
 * ============================================================================ */

/**
 * @brief Mock WebSocket connection structure
 */
typedef struct {
    websocket_state_t state;
    char url[256];
    char auth_token[128];
    int port;
    bool ssl_enabled;
    time_t last_activity;
} mock_websocket_t;

/**
 * @brief Marine sensor data structure
 */
typedef struct {
    marine_sensor_type_t type;
    char path[128];
    union {
        struct {
            double latitude;
            double longitude;
        } position;
        struct {
            double speed_ms;
            double direction_deg;
        } wind;
        double value;
    } data;
    time_t timestamp;
    bool valid;
} marine_sensor_data_t;

/**
 * @brief SignalK message structure
 */
typedef struct {
    char context[64];
    char source[64];
    time_t timestamp;
    marine_sensor_data_t* values;
    size_t value_count;
} signalk_message_t;

/* ============================================================================
 * Test Utility Functions
 * ============================================================================ */

/**
 * @brief Create mock SignalK message for testing
 * @param sensor_type Type of sensor data to include
 * @return Allocated SignalK message (caller must free)
 */
signalk_message_t* create_mock_signalk_message(marine_sensor_type_t sensor_type);

/**
 * @brief Free SignalK message
 * @param message Message to free
 */
void free_signalk_message(signalk_message_t* message);

/**
 * @brief Validate SignalK message structure
 * @param message Message to validate
 * @return true if valid, false otherwise
 */
bool validate_signalk_message(const signalk_message_t* message);

/**
 * @brief Generate random marine sensor data
 * @param sensor_type Type of sensor
 * @return Generated sensor data
 */
marine_sensor_data_t generate_random_sensor_data(marine_sensor_type_t sensor_type);

/**
 * @brief Convert sensor type to SignalK path
 * @param sensor_type Sensor type
 * @param path_buffer Buffer to store path (min 128 bytes)
 * @return true on success, false on error
 */
bool sensor_type_to_signalk_path(marine_sensor_type_t sensor_type, char* path_buffer);

/**
 * @brief Create test configuration file
 * @param filename Configuration file name
 * @param include_auth Whether to include authentication
 * @return true on success, false on error
 */
bool create_test_config_file(const char* filename, bool include_auth);

/**
 * @brief Cleanup test configuration file
 * @param filename Configuration file name
 */
void cleanup_test_config_file(const char* filename);

/* ============================================================================
 * Time and Timing Utilities
 * ============================================================================ */

/**
 * @brief Get current time in milliseconds since epoch
 * @return Time in milliseconds
 */
uint64_t get_current_time_ms(void);

/**
 * @brief Wait for condition with timeout
 * @param condition Function to check condition
 * @param timeout_ms Timeout in milliseconds
 * @param check_interval_ms Check interval in milliseconds
 * @return true if condition met, false if timeout
 */
bool wait_for_condition_timeout(bool (*condition)(void), 
                               uint32_t timeout_ms, 
                               uint32_t check_interval_ms);

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void sleep_ms(uint32_t ms);

/* ============================================================================
 * String and Data Utilities
 * ============================================================================ */

/**
 * @brief Safe string copy with bounds checking
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return true on success, false if truncated
 */
bool safe_string_copy(char* dest, const char* src, size_t dest_size);

/**
 * @brief Generate random string for testing
 * @param buffer Buffer to store string
 * @param length Length of string to generate
 * @param include_numbers Include numbers in string
 */
void generate_random_string(char* buffer, size_t length, bool include_numbers);

/**
 * @brief Compare floating point values with tolerance
 * @param a First value
 * @param b Second value
 * @param tolerance Tolerance for comparison
 * @return true if values are equal within tolerance
 */
bool float_equals_tolerance(double a, double b, double tolerance);

/* ============================================================================
 * File and Directory Utilities
 * ============================================================================ */

/**
 * @brief Check if file exists
 * @param filename File to check
 * @return true if file exists, false otherwise
 */
bool file_exists(const char* filename);

/**
 * @brief Create temporary directory for testing
 * @param dir_path Buffer to store directory path
 * @param path_size Size of directory path buffer
 * @return true on success, false on error
 */
bool create_temp_directory(char* dir_path, size_t path_size);

/**
 * @brief Remove directory and all contents
 * @param dir_path Directory to remove
 * @return true on success, false on error
 */
bool remove_directory_recursive(const char* dir_path);

/* ============================================================================
 * Memory Management Utilities
 * ============================================================================ */

/**
 * @brief Allocate memory with error checking
 * @param size Size to allocate
 * @return Allocated memory or NULL on error
 */
void* test_malloc(size_t size);

/**
 * @brief Free memory allocated with test_malloc
 * @param ptr Pointer to free
 */
void test_free(void* ptr);

/**
 * @brief Get current memory usage (approximate)
 * @return Memory usage in bytes
 */
size_t get_memory_usage(void);

/* ============================================================================
 * Network Test Utilities
 * ============================================================================ */

/**
 * @brief Check if port is available for binding
 * @param port Port number to check
 * @return true if port is available, false otherwise
 */
bool is_port_available(int port);

/**
 * @brief Find available port in range
 * @param start_port Starting port number
 * @param end_port Ending port number
 * @return Available port number or -1 if none found
 */
int find_available_port(int start_port, int end_port);

/**
 * @brief Simulate network latency for testing
 * @param latency_ms Latency to simulate in milliseconds
 */
void simulate_network_latency(uint32_t latency_ms);

#endif /* SIGNALK_TEST_UTILS_H */
