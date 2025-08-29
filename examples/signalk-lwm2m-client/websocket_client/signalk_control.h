#ifndef SIGNALK_CONTROL_H
#define SIGNALK_CONTROL_H

#include <stdbool.h>

/**
 * @file signalk_control.h
 * @brief SignalK PUT command integration for vessel system control
 * 
 * This module provides HTTP-based SignalK PUT command functionality to control
 * vessel systems from LwM2M write operations. It enables bidirectional communication
 * where LwM2M servers can control vessel equipment via SignalK PUT requests.
 */

/**
 * Configuration for SignalK control operations
 */
typedef struct {
    char server_host[256];      ///< SignalK server hostname/IP
    int server_port;            ///< SignalK server HTTP port
    char vessel_id[128];        ///< Vessel identifier (default: "self")
    int timeout_ms;             ///< HTTP request timeout in milliseconds
    bool verify_ssl;            ///< SSL certificate verification (for HTTPS)
} signalk_control_config_t;

/**
 * Result of SignalK PUT operation
 */
typedef enum {
    SIGNALK_PUT_SUCCESS = 0,    ///< PUT command successful
    SIGNALK_PUT_ERROR_NETWORK,  ///< Network/connection error
    SIGNALK_PUT_ERROR_HTTP,     ///< HTTP error response
    SIGNALK_PUT_ERROR_JSON,     ///< JSON formatting error
    SIGNALK_PUT_ERROR_TIMEOUT,  ///< Request timeout
    SIGNALK_PUT_ERROR_CONFIG    ///< Configuration error
} signalk_put_result_t;

/**
 * Initialize SignalK control system
 * @param config Configuration parameters for SignalK control
 * @return true on success, false on failure
 */
bool signalk_control_init(const signalk_control_config_t *config);

/**
 * Send SignalK PUT command to control a switch
 * @param switch_path SignalK path for the switch (e.g., "electrical.switches.navigation.lights")
 * @param state Switch state (true = on, false = off)
 * @return Result of the PUT operation
 */
signalk_put_result_t signalk_control_switch(const char *switch_path, bool state);

/**
 * Send SignalK PUT command to control a dimmer
 * @param dimmer_path SignalK path for the dimmer (e.g., "electrical.switches.cabin.lights")
 * @param dimmer_value Dimmer value (0-100)
 * @return Result of the PUT operation
 */
signalk_put_result_t signalk_control_dimmer(const char *dimmer_path, int dimmer_value);

/**
 * Send SignalK PUT command with numeric value
 * @param path SignalK path
 * @param value Numeric value to set
 * @return Result of the PUT operation
 */
signalk_put_result_t signalk_control_numeric(const char *path, double value);

/**
 * Send SignalK PUT command with string value
 * @param path SignalK path
 * @param value String value to set
 * @return Result of the PUT operation
 */
signalk_put_result_t signalk_control_string(const char *path, const char *value);

/**
 * Get human-readable error description
 * @param result PUT operation result
 * @return Error description string
 */
const char *signalk_control_error_string(signalk_put_result_t result);

/**
 * Cleanup SignalK control system
 */
void signalk_control_cleanup(void);

/**
 * Test SignalK control connectivity
 * @return true if SignalK server is reachable, false otherwise
 */
bool signalk_control_test_connection(void);

/**
 * Load SignalK control configuration from settings JSON
 * @param config_file Path to settings.json file
 * @return true on success, false on failure
 */
bool signalk_control_load_config(const char *config_file);

#endif // SIGNALK_CONTROL_H
