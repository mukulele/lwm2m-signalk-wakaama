#ifndef SIGNALK_AUTH_H
#define SIGNALK_AUTH_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * @file signalk_auth.h
 * @brief SignalK authentication module for secure PUT requests
 * 
 * Implements token-based authentication as per SignalK v1.7.0 specification
 * Handles login, token renewal, and secure messaging for vessel control
 */

// Maximum lengths for authentication data
#define SIGNALK_TOKEN_MAX_LEN 512
#define SIGNALK_USERNAME_MAX_LEN 64
#define SIGNALK_PASSWORD_MAX_LEN 64
#define SIGNALK_REQUEST_ID_MAX_LEN 32

/**
 * Authentication state enumeration
 */
typedef enum {
    SIGNALK_AUTH_DISCONNECTED = 0,    ///< Not connected
    SIGNALK_AUTH_CONNECTING,          ///< Attempting connection
    SIGNALK_AUTH_AUTHENTICATING,      ///< Sending login request
    SIGNALK_AUTH_AUTHENTICATED,       ///< Successfully authenticated
    SIGNALK_AUTH_TOKEN_EXPIRED,       ///< Token needs renewal
    SIGNALK_AUTH_FAILED              ///< Authentication failed
} signalk_auth_state_t;

/**
 * Authentication configuration structure
 */
typedef struct {
    bool enabled;                                    ///< Authentication enabled flag
    char username[SIGNALK_USERNAME_MAX_LEN];         ///< SignalK username
    char password[SIGNALK_PASSWORD_MAX_LEN];         ///< SignalK password
    uint32_t token_renewal_time;                     ///< Token renewal time in seconds
} signalk_auth_config_t;

/**
 * Authentication context structure
 */
typedef struct {
    signalk_auth_config_t config;                   ///< Authentication configuration
    signalk_auth_state_t state;                     ///< Current authentication state
    char token[SIGNALK_TOKEN_MAX_LEN];              ///< Current JWT token
    time_t token_expires;                           ///< Token expiration timestamp
    char request_id[SIGNALK_REQUEST_ID_MAX_LEN];    ///< Current request ID
    uint32_t request_counter;                       ///< Request counter for unique IDs
} signalk_auth_context_t;

/**
 * Initialize the SignalK authentication module
 * @param config Authentication configuration
 * @return true on success, false on failure
 */
bool signalk_auth_init(const signalk_auth_config_t *config);

/**
 * Get the current authentication state
 * @return Current authentication state
 */
signalk_auth_state_t signalk_auth_get_state(void);

/**
 * Check if authentication is enabled and required
 * @return true if authentication is enabled
 */
bool signalk_auth_is_enabled(void);

/**
 * Check if currently authenticated with valid token
 * @return true if authenticated with valid token
 */
bool signalk_auth_is_authenticated(void);

/**
 * Generate a login message for WebSocket authentication
 * @param buffer Output buffer for JSON message
 * @param buffer_size Size of output buffer
 * @return Length of generated message, 0 on error
 */
size_t signalk_auth_generate_login_message(char *buffer, size_t buffer_size);

/**
 * Process authentication response from SignalK server
 * @param json_response JSON response from server
 * @return true on successful authentication, false on failure
 */
bool signalk_auth_process_response(const char *json_response);

/**
 * Get current authentication token for including in messages
 * @return Pointer to current token, NULL if not authenticated
 */
const char* signalk_auth_get_token(void);

/**
 * Add authentication token to SignalK message
 * @param message_buffer Message buffer to modify
 * @param buffer_size Size of message buffer
 * @return true on success, false if buffer too small
 */
bool signalk_auth_add_token_to_message(char *message_buffer, size_t buffer_size);

/**
 * Check if token needs renewal and generate renewal message if needed
 * @param buffer Output buffer for renewal message
 * @param buffer_size Size of output buffer
 * @return Length of renewal message, 0 if no renewal needed
 */
size_t signalk_auth_check_token_renewal(char *buffer, size_t buffer_size);

/**
 * Generate logout message
 * @param buffer Output buffer for JSON message
 * @param buffer_size Size of output buffer
 * @return Length of generated message, 0 on error
 */
size_t signalk_auth_generate_logout_message(char *buffer, size_t buffer_size);

/**
 * Reset authentication state (for reconnection scenarios)
 */
void signalk_auth_reset(void);

/**
 * Cleanup authentication module
 */
void signalk_auth_cleanup(void);

#endif // SIGNALK_AUTH_H
