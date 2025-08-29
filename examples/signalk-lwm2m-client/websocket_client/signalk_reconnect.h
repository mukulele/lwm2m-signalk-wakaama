#ifndef SIGNALK_RECONNECT_H
#define SIGNALK_RECONNECT_H

#include <stdbool.h>
#include <time.h>

/**
 * @file signalk_reconnect.h
 * @brief Automatic reconnection with exponential backoff for SignalK WebSocket connections
 * 
 * This module provides robust connection management for marine IoT applications
 * where network reliability is critical but connections may drop due to various
 * factors like weather, distance, or server maintenance.
 */

/**
 * Reconnection configuration
 */
typedef struct {
    bool auto_reconnect_enabled;    ///< Enable automatic reconnection
    int max_retries;                ///< Maximum number of retry attempts (0 = infinite)
    int base_delay_ms;              ///< Initial delay between retries (milliseconds)
    int max_delay_ms;               ///< Maximum delay between retries (milliseconds)
    double backoff_multiplier;      ///< Multiplier for exponential backoff (default: 2.0)
    int jitter_percent;             ///< Add random jitter to prevent thundering herd (0-100%)
    int connection_timeout_ms;      ///< Timeout for individual connection attempts
    bool reset_on_success;          ///< Reset retry count on successful connection
} signalk_reconnect_config_t;

/**
 * Connection state information
 */
typedef struct {
    bool is_connected;              ///< Current connection status
    int retry_count;                ///< Current number of retries
    time_t last_attempt;            ///< Timestamp of last connection attempt
    time_t last_success;            ///< Timestamp of last successful connection
    int next_delay_ms;              ///< Calculated delay for next retry
    char last_error[256];           ///< Description of last connection error
} signalk_connection_state_t;

/**
 * Connection attempt result
 */
typedef enum {
    SIGNALK_CONNECT_SUCCESS = 0,    ///< Connection successful
    SIGNALK_CONNECT_FAILED,         ///< Connection failed (will retry)
    SIGNALK_CONNECT_TIMEOUT,        ///< Connection timeout
    SIGNALK_CONNECT_MAX_RETRIES,    ///< Maximum retries exceeded
    SIGNALK_CONNECT_DISABLED        ///< Auto-reconnect disabled
} signalk_connect_result_t;

/**
 * Initialize the reconnection system
 * @param config Reconnection configuration parameters
 * @return true on success, false on failure
 */
bool signalk_reconnect_init(const signalk_reconnect_config_t *config);

/**
 * Load reconnection configuration from settings.json
 * @param config_file Path to settings.json file
 * @return true on success, false on failure
 */
bool signalk_reconnect_load_config(const char *config_file);

/**
 * Attempt connection with automatic retry logic
 * @param server SignalK server hostname/IP
 * @param port SignalK server port
 * @return Connection result code
 */
signalk_connect_result_t signalk_reconnect_attempt(const char *server, int port);

/**
 * Handle connection loss and schedule reconnection
 * Called when WebSocket connection is lost
 */
void signalk_reconnect_on_disconnect(void);

/**
 * Notify successful connection
 * Resets retry counters and state
 */
void signalk_reconnect_on_connect(void);

/**
 * Check if reconnection should be attempted now
 * @return true if enough time has passed for next retry attempt
 */
bool signalk_reconnect_should_retry(void);

/**
 * Get current connection state information
 * @return Pointer to connection state structure
 */
const signalk_connection_state_t* signalk_reconnect_get_state(void);

/**
 * Get human-readable description of connection result
 * @param result Connection result code
 * @return Error description string
 */
const char* signalk_reconnect_error_string(signalk_connect_result_t result);

/**
 * Reset reconnection state
 * Clears retry counts and error state
 */
void signalk_reconnect_reset(void);

/**
 * Cleanup reconnection system
 */
void signalk_reconnect_cleanup(void);

/**
 * Get default reconnection configuration for marine IoT
 * Optimized for marine environments with potentially unstable connectivity
 * @return Default configuration structure
 */
signalk_reconnect_config_t signalk_reconnect_get_default_config(void);

/**
 * Calculate next retry delay using exponential backoff
 * @param attempt_number Current attempt number (1-based)
 * @return Delay in milliseconds for next attempt
 */
int signalk_reconnect_calculate_delay(int attempt_number);

/**
 * Check if auto-reconnect is enabled
 * @return true if auto-reconnect is enabled
 */
bool signalk_reconnect_is_enabled(void);

/**
 * Enable or disable auto-reconnect
 * @param enabled New auto-reconnect state
 */
void signalk_reconnect_set_enabled(bool enabled);

#endif // SIGNALK_RECONNECT_H
