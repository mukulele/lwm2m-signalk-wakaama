/*******************************************************************************
 * WebSocket Mock Header
 * Purpose: Header file for WebSocket mock implementation
 * 
 * Provides function declarations for comprehensive WebSocket testing
 *******************************************************************************/

#ifndef WEBSOCKET_MOCK_H
#define WEBSOCKET_MOCK_H

#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Core WebSocket Mock Functions
 *******************************************************************************/

/**
 * Initialize the WebSocket mock system
 * @return 0 on success, negative on error
 */
int websocket_mock_init(void);

/**
 * Cleanup the WebSocket mock system and free resources
 */
void websocket_mock_cleanup(void);

/**
 * Check if WebSocket is currently connected
 * @return true if connected, false otherwise
 */
bool websocket_mock_is_connected(void);

/**
 * Connect to a WebSocket server
 * @param server Server hostname or IP address
 * @param port Server port number
 * @param path WebSocket path (e.g., "/signalk/v1/stream")
 * @return 0 on success, negative on error
 */
int websocket_mock_connect(const char* server, int port, const char* path);

/**
 * Connect to a WebSocket server with authentication
 * @param server Server hostname or IP address
 * @param port Server port number
 * @param path WebSocket path
 * @param auth_token Authentication token (JWT, etc.)
 * @return 0 on success, negative on error
 */
int websocket_mock_connect_with_auth(const char* server, int port, const char* path, const char* auth_token);

/**
 * Disconnect from WebSocket server
 */
void websocket_mock_disconnect(void);

/**
 * Send data through WebSocket connection
 * @param data JSON string or data to send
 * @return 0 on success, negative on error
 */
int websocket_mock_send_data(const char* data);

/*******************************************************************************
 * Subscription Management
 *******************************************************************************/

/**
 * Subscribe to SignalK data paths
 * @param path_pattern Path pattern to subscribe to (e.g., "navigation.*")
 * @return 0 on success, negative on error
 */
int websocket_mock_subscribe(const char* path_pattern);

/**
 * Unsubscribe from SignalK data paths
 * @param path_pattern Path pattern to unsubscribe from
 * @return 0 on success, negative on error
 */
int websocket_mock_unsubscribe(const char* path_pattern);

/*******************************************************************************
 * Testing and Simulation Functions
 *******************************************************************************/

/**
 * Simulate network error conditions
 */
void websocket_mock_simulate_network_error(void);

/**
 * Clear simulated network error conditions
 */
void websocket_mock_clear_network_error(void);

/**
 * Get count of messages sent during session
 * @return Number of messages sent
 */
int websocket_mock_get_message_count(void);

/**
 * Get count of active subscriptions
 * @return Number of active subscriptions
 */
int websocket_mock_get_subscription_count(void);

/**
 * Get current connection information
 * @param server Buffer to store server name (optional, can be NULL)
 * @param port Pointer to store port number (optional, can be NULL)
 * @param path Buffer to store path (optional, can be NULL)
 */
void websocket_mock_get_connection_info(char* server, int* port, char* path);

/*******************************************************************************
 * Error Codes
 *******************************************************************************/

#define WEBSOCKET_MOCK_SUCCESS           0
#define WEBSOCKET_MOCK_ERROR_INVALID    -1
#define WEBSOCKET_MOCK_ERROR_AUTH       -2
#define WEBSOCKET_MOCK_ERROR_TIMEOUT    -3
#define WEBSOCKET_MOCK_ERROR_NETWORK    -4
#define WEBSOCKET_MOCK_ERROR_NOT_CONN   -5

#ifdef __cplusplus
}
#endif

#endif /* WEBSOCKET_MOCK_H */
