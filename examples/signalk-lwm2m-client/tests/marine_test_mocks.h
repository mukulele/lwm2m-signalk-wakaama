/**
 * @file marine_test_mocks.h
 * @brief Marine IoT Test Mocks and Stubs
 * 
 * Mock implementations for marine IoT components following
 * Wakaama testing standards.
 * 
 * @author SignalK-LwM2M Integration Team
 * @date 2025
 */

#ifndef MARINE_TEST_MOCKS_H
#define MARINE_TEST_MOCKS_H

#include "signalk_test_utils.h"
#include "liblwm2m.h"

/* ============================================================================
 * Mock Server Structures
 * ============================================================================ */

/**
 * @brief Mock SignalK server structure
 */
typedef struct {
    int port;
    bool running;
    char auth_token[128];
    marine_sensor_data_t* sensors;
    size_t sensor_count;
    size_t max_sensors;
    uint64_t message_count;
    uint64_t client_count;
} mock_signalk_server_t;

/**
 * @brief Mock LwM2M server structure
 */
typedef struct {
    int port;
    bool running;
    lwm2m_object_t* objects;
    size_t object_count;
    size_t max_objects;
    uint64_t registration_count;
} mock_lwm2m_server_t;

/* ============================================================================
 * Mock Implementation Functions
 * ============================================================================ */

/* Memory tracking for leak detection */
void test_memory_leak_detection_start(void);
size_t test_memory_leak_detection_check(void);

/* Mock SignalK server functions */
void* test_create_mock_signalk_server(int port);
void test_destroy_mock_signalk_server(void* server);
bool test_mock_signalk_server_add_sensor(void* server, const marine_sensor_data_t* sensor);
bool test_mock_signalk_server_remove_sensor(void* server, marine_sensor_type_t sensor_type);
size_t test_mock_signalk_server_get_message_count(void* server);

/* Mock LwM2M functions */
lwm2m_object_t* test_create_mock_lwm2m_object(uint16_t object_id, uint16_t instance_id);
void test_destroy_mock_lwm2m_object(lwm2m_object_t* object);
bool test_validate_lwm2m_object(lwm2m_object_t* object, uint16_t expected_obj_id);

/* Network simulation */
void test_simulate_network_conditions(bool simulate_disconnect);

/* Performance benchmarking */
// ...existing code...

/* Test condition waiting */
bool test_wait_for_condition(bool (*condition)(void), int timeout_sec);

/* Marine sensor data creation */
marine_sensor_data_t* test_create_marine_sensor_data(marine_sensor_type_t sensor_type);

#endif /* MARINE_TEST_MOCKS_H */
