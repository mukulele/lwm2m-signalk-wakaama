/**
 * @file mocks/signalk_mock.c
 * @brief SignalK Mock Implementation
 * 
 * Mock SignalK server and client functions for testing.
 * 
 * @author SignalK-LwM2M Integration Team
 * @date 2025
 */

#include "../marine_test_mocks.h"
#include <stdint.h>
#include <unistd.h>
#include "../test_framework.h"
#include <sys/time.h>
#include <pthread.h>

/* ============================================================================
 * Global Mock State
 * ============================================================================ */

static mock_signalk_server_t* g_mock_server = NULL;
static bool g_network_disconnected = false;
static size_t g_allocated_bytes = 0;

/* ============================================================================
 * Memory Tracking Implementation
 * ============================================================================ */

void test_memory_leak_detection_start(void)
{
    g_allocated_bytes = 0;
}

size_t test_memory_leak_detection_check(void)
{
    return g_allocated_bytes;
}

static void* tracked_malloc(size_t size)
{
    void* ptr = malloc(size);
    if (ptr) {
        g_allocated_bytes += size;
    }
    return ptr;
}

static void tracked_free(void* ptr, size_t size)
{
    if (ptr) {
        g_allocated_bytes -= size;
        free(ptr);
    }
}

/* ============================================================================
 * SignalK Mock Server Implementation
 * ============================================================================ */

void* test_create_mock_signalk_server(int port)
{
    mock_signalk_server_t* server = tracked_malloc(sizeof(mock_signalk_server_t));
    if (!server) {
        return NULL;
    }
    
    memset(server, 0, sizeof(mock_signalk_server_t));
    server->port = port;
    server->running = true;
    server->max_sensors = 32;
    server->sensors = tracked_malloc(sizeof(marine_sensor_data_t) * server->max_sensors);
    
    if (!server->sensors) {
        tracked_free(server, sizeof(mock_signalk_server_t));
        return NULL;
    }
    
    /* Set default auth token */
    strncpy(server->auth_token, TEST_SIGNALK_AUTH_TOKEN, sizeof(server->auth_token) - 1);
    
    g_mock_server = server;
    return server;
}

void test_destroy_mock_signalk_server(void* server_ptr)
{
    if (g_mock_server) {
        if (g_mock_server->sensors) {
            tracked_free(g_mock_server->sensors, 
                        sizeof(marine_sensor_data_t) * g_mock_server->max_sensors);
        }
        tracked_free(g_mock_server, sizeof(mock_signalk_server_t));
        g_mock_server = NULL;
    }
}

bool test_mock_signalk_server_add_sensor(void* server_ptr, const marine_sensor_data_t* sensor)
{
    mock_signalk_server_t* server = (mock_signalk_server_t*)server_ptr;
    if (!server || !sensor || server->sensor_count >= server->max_sensors) {
        return false;
    }
    
    memcpy(&server->sensors[server->sensor_count], sensor, sizeof(marine_sensor_data_t));
    server->sensor_count++;
    server->message_count++;
    
    return true;
}

bool test_mock_signalk_server_remove_sensor(void* server_ptr, marine_sensor_type_t sensor_type)
{
    mock_signalk_server_t* server = (mock_signalk_server_t*)server_ptr;
    if (!server) {
        return false;
    }
    
    for (size_t i = 0; i < server->sensor_count; i++) {
        if (server->sensors[i].type == sensor_type) {
            /* Move remaining sensors */
            for (size_t j = i; j < server->sensor_count - 1; j++) {
                memcpy(&server->sensors[j], &server->sensors[j + 1], sizeof(marine_sensor_data_t));
            }
            server->sensor_count--;
            return true;
        }
    }
    
    return false;
}

size_t test_mock_signalk_server_get_message_count(void* server_ptr)
{
    mock_signalk_server_t* server = (mock_signalk_server_t*)server_ptr;
    return server ? server->message_count : 0;
}

/* ============================================================================
 * Network Simulation Implementation
 * ============================================================================ */

void test_simulate_network_conditions(bool simulate_disconnect)
{
    g_network_disconnected = simulate_disconnect;
    
    if (simulate_disconnect && g_mock_server) {
        g_mock_server->running = false;
    } else if (!simulate_disconnect && g_mock_server) {
        g_mock_server->running = true;
    }
}

/* ============================================================================
 * Performance Benchmarking Implementation
 * ============================================================================ */

void test_benchmark_start(test_benchmark_t* benchmark, const char* operation_name)
{
    if (!benchmark) return;
    
    benchmark->operation_name = operation_name;
    benchmark->memory_usage = g_allocated_bytes;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    benchmark->start_time = (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

void test_benchmark_end(test_benchmark_t* benchmark)
{
    if (!benchmark) return;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    benchmark->end_time = (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
    benchmark->duration_ms = benchmark->end_time - benchmark->start_time;
    benchmark->memory_usage = g_allocated_bytes - benchmark->memory_usage;
}

void test_benchmark_print(const test_benchmark_t* benchmark)
{
    if (!benchmark) return;
    
    printf("  🕒 Benchmark: %s\n", benchmark->operation_name);
    printf("     Duration: %.2f ms\n", benchmark->duration_ms);
    printf("     Memory: %zu bytes\n", benchmark->memory_usage);
}

/* ============================================================================
 * Condition Waiting Implementation
 * ============================================================================ */

bool test_wait_for_condition(bool (*condition)(void), int timeout_sec)
{
    if (!condition) return false;
    
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < timeout_sec) {
        if (condition()) {
            return true;
        }
        usleep(100000); /* 100ms */
    }
    
    return false;
}

/* ============================================================================
 * Marine Sensor Data Creation
 * ============================================================================ */

marine_sensor_data_t* test_create_marine_sensor_data(marine_sensor_type_t sensor_type)
{
    marine_sensor_data_t* sensor_data = tracked_malloc(sizeof(marine_sensor_data_t));
    if (!sensor_data) {
        return NULL;
    }
    
    memset(sensor_data, 0, sizeof(marine_sensor_data_t));
    sensor_data->type = sensor_type;
    sensor_data->timestamp = time(NULL);
    sensor_data->valid = true;
    
    switch (sensor_type) {
        case SENSOR_TYPE_GPS:
            strncpy(sensor_data->path, "navigation.position", sizeof(sensor_data->path) - 1);
            sensor_data->data.position.latitude = 52.0907 + ((rand() % 1000) - 500) / 100000.0;
            sensor_data->data.position.longitude = 5.1214 + ((rand() % 1000) - 500) / 100000.0;
            break;
            
        case SENSOR_TYPE_WIND:
            strncpy(sensor_data->path, "environment.wind.speedApparent", sizeof(sensor_data->path) - 1);
            sensor_data->data.wind.speed_ms = (rand() % 20) + (rand() % 100) / 100.0;
            sensor_data->data.wind.direction_deg = rand() % 360;
            break;
            
        case SENSOR_TYPE_DEPTH:
            strncpy(sensor_data->path, "environment.depth.belowKeel", sizeof(sensor_data->path) - 1);
            sensor_data->data.value = (rand() % 50) + (rand() % 100) / 100.0;
            break;
            
        case SENSOR_TYPE_TEMPERATURE:
            strncpy(sensor_data->path, "environment.water.temperature", sizeof(sensor_data->path) - 1);
            sensor_data->data.value = 15.0 + (rand() % 20) + (rand() % 100) / 100.0;
            break;
            
        default:
            sensor_data->data.value = rand() % 100;
            strncpy(sensor_data->path, "unknown.sensor", sizeof(sensor_data->path) - 1);
            break;
    }
    
    return sensor_data;
}
