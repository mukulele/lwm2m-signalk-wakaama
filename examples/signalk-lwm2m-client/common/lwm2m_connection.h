#ifndef LWM2M_CONNECTION_H
#define LWM2M_CONNECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include "liblwm2m.h"
#ifdef WITH_TINYDTLS
#include "tinydtls/connection.h"
#else
#include "udp/connection.h"
#endif

/**
 * @file lwm2m_connection.h
 * @brief LwM2M connection management module
 * 
 * This module encapsulates LwM2M client connection lifecycle, state management,
 * and communication handling for better code organization and maintainability.
 */

// Forward declarations - avoid conflicts with existing definitions

/**
 * LwM2M connection configuration
 */
typedef struct {
    const char *server_host;        ///< LwM2M server hostname
    const char *server_port;        ///< LwM2M server port
    const char *local_port;         ///< Local UDP port for client
    const char *endpoint_name;      ///< Client endpoint name
    int lifetime;                   ///< Registration lifetime in seconds
    int address_family;             ///< AF_INET or AF_INET6
    bool bootstrap_requested;       ///< Whether bootstrap is requested
    uint16_t coap_block_size;       ///< CoAP block size
    char *psk_id;                   ///< Pre-shared key ID (optional)
    char *psk;                      ///< Pre-shared key (optional)
    uint16_t psk_len;               ///< PSK length
} lwm2m_connection_config_t;

/**
 * LwM2M connection state information
 */
typedef struct {
    lwm2m_context_t *context;       ///< LwM2M context
    client_data_t *client_data;     ///< Client data structure
    lwm2m_connection_config_t config; ///< Connection configuration
    bool is_running;                ///< Whether connection is active
    time_t reboot_time;             ///< Scheduled reboot time (if any)
} lwm2m_connection_state_t;

/**
 * Initialize LwM2M connection module
 * @param config Connection configuration parameters
 * @return Pointer to connection state, NULL on failure
 */
lwm2m_connection_state_t* lwm2m_connection_init(const lwm2m_connection_config_t *config);

/**
 * Start the LwM2M connection
 * @param state Connection state
 * @return 0 on success, non-zero on failure
 */
int lwm2m_connection_start(lwm2m_connection_state_t *state);

/**
 * Process one iteration of the LwM2M connection loop
 * @param state Connection state
 * @param timeout_ms Maximum time to wait for events (milliseconds)
 * @return 0 to continue, non-zero to stop
 */
int lwm2m_connection_process(lwm2m_connection_state_t *state, int timeout_ms);

/**
 * Check if LwM2M connection is ready (STATE_READY)
 * @param state Connection state
 * @return true if ready, false otherwise
 */
bool lwm2m_connection_is_ready(const lwm2m_connection_state_t *state);

/**
 * Get current LwM2M state
 * @param state Connection state
 * @return Current lwm2m_client_state_t
 */
lwm2m_client_state_t lwm2m_connection_get_state(const lwm2m_connection_state_t *state);

/**
 * Handle command input for LwM2M client
 * @param state Connection state
 * @param command Command string to process
 * @return 0 on success, non-zero on failure
 */
int lwm2m_connection_handle_command(lwm2m_connection_state_t *state, const char *command);

/**
 * Stop and cleanup LwM2M connection
 * @param state Connection state (will be freed)
 */
void lwm2m_connection_cleanup(lwm2m_connection_state_t *state);

/**
 * Check if reboot is scheduled and handle it
 * @param state Connection state
 * @return true if reboot is imminent, false otherwise
 */
bool lwm2m_connection_handle_reboot(lwm2m_connection_state_t *state);

/**
 * Get the socket file descriptor for select() monitoring
 * @param state Connection state
 * @return Socket file descriptor, -1 on error
 */
int lwm2m_connection_get_socket(const lwm2m_connection_state_t *state);

#endif // LWM2M_CONNECTION_H
