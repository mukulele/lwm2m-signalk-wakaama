#include "signalk_auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// JSON parsing for simple token extraction
#include <cjson/cJSON.h>

// Global authentication context
static signalk_auth_context_t g_auth_context = {0};
static bool g_auth_initialized = false;

/**
 * Generate unique request ID
 */
static void generate_request_id(char *request_id, size_t max_len) {
    g_auth_context.request_counter++;
    snprintf(request_id, max_len, "lwm2m-auth-%u-%lu", 
             g_auth_context.request_counter, (unsigned long)time(NULL));
}

bool signalk_auth_init(const signalk_auth_config_t *config) {
    if (!config) {
        return false;
    }
    
    // Copy configuration
    memcpy(&g_auth_context.config, config, sizeof(signalk_auth_config_t));
    
    // Initialize state
    g_auth_context.state = SIGNALK_AUTH_DISCONNECTED;
    g_auth_context.token[0] = '\0';
    g_auth_context.token_expires = 0;
    g_auth_context.request_counter = 0;
    
    g_auth_initialized = true;
    
    printf("[SignalK Auth] Initialized - Authentication %s\n", 
           config->enabled ? "ENABLED" : "DISABLED");
    
    return true;
}

signalk_auth_state_t signalk_auth_get_state(void) {
    return g_auth_context.state;
}

bool signalk_auth_is_enabled(void) {
    return g_auth_initialized && g_auth_context.config.enabled;
}

bool signalk_auth_is_authenticated(void) {
    if (!signalk_auth_is_enabled()) {
        return true; // No auth required
    }
    
    return (g_auth_context.state == SIGNALK_AUTH_AUTHENTICATED) && 
           (g_auth_context.token[0] != '\0') &&
           (time(NULL) < g_auth_context.token_expires);
}

size_t signalk_auth_generate_login_message(char *buffer, size_t buffer_size) {
    if (!buffer || !signalk_auth_is_enabled()) {
        return 0;
    }
    
    // Generate unique request ID
    generate_request_id(g_auth_context.request_id, sizeof(g_auth_context.request_id));
    
    // Build login message according to SignalK spec
    int result = snprintf(buffer, buffer_size,
        "{"
        "\"requestId\":\"%s\","
        "\"login\":{"
        "\"username\":\"%s\","
        "\"password\":\"%s\""
        "}"
        "}",
        g_auth_context.request_id,
        g_auth_context.config.username,
        g_auth_context.config.password
    );
    
    if (result > 0 && (size_t)result < buffer_size) {
        g_auth_context.state = SIGNALK_AUTH_AUTHENTICATING;
        printf("[SignalK Auth] Generated login message for user: %s\n", 
               g_auth_context.config.username);
        return (size_t)result;
    }
    
    return 0;
}

bool signalk_auth_process_response(const char *json_response) {
    if (!json_response || !signalk_auth_is_enabled()) {
        return false;
    }
    
    // Parse JSON response
    cJSON *root = cJSON_Parse(json_response);
    if (!root) {
        printf("[SignalK Auth] Failed to parse authentication response\n");
        return false;
    }
    
    // Check if this is our response
    cJSON *request_id_obj = cJSON_GetObjectItem(root, "requestId");
    if (!request_id_obj || !cJSON_IsString(request_id_obj)) {
        cJSON_Delete(root);
        return false; // Not an auth response
    }
    
    const char *response_request_id = cJSON_GetStringValue(request_id_obj);
    if (strcmp(response_request_id, g_auth_context.request_id) != 0) {
        cJSON_Delete(root);
        return false; // Not our response
    }
    
    // Check result code
    cJSON *result_obj = cJSON_GetObjectItem(root, "result");
    if (!result_obj || !cJSON_IsNumber(result_obj)) {
        cJSON_Delete(root);
        return false;
    }
    
    int result_code = (int)cJSON_GetNumberValue(result_obj);
    if (result_code != 200) {
        printf("[SignalK Auth] Authentication failed with code: %d\n", result_code);
        g_auth_context.state = SIGNALK_AUTH_FAILED;
        cJSON_Delete(root);
        return false;
    }
    
    // Extract login token data
    cJSON *login_obj = cJSON_GetObjectItem(root, "login");
    if (!login_obj || !cJSON_IsObject(login_obj)) {
        cJSON_Delete(root);
        return false;
    }
    
    // Get token
    cJSON *token_obj = cJSON_GetObjectItem(login_obj, "token");
    if (!token_obj || !cJSON_IsString(token_obj)) {
        cJSON_Delete(root);
        return false;
    }
    
    const char *token = cJSON_GetStringValue(token_obj);
    if (!token || strlen(token) >= SIGNALK_TOKEN_MAX_LEN) {
        printf("[SignalK Auth] Invalid token in response\n");
        cJSON_Delete(root);
        return false;
    }
    
    // Get time to live
    cJSON *ttl_obj = cJSON_GetObjectItem(login_obj, "timeToLive");
    int ttl = g_auth_context.config.token_renewal_time; // Default fallback
    if (ttl_obj && cJSON_IsNumber(ttl_obj)) {
    }
    
    // Store authentication data
    strncpy(g_auth_context.token, token, sizeof(g_auth_context.token) - 1);
    g_auth_context.token[sizeof(g_auth_context.token) - 1] = '\0';
    g_auth_context.token_expires = time(NULL) + ttl;
    g_auth_context.state = SIGNALK_AUTH_AUTHENTICATED;
    
    printf("[SignalK Auth] Successfully authenticated! Token expires in %d seconds\n", ttl);
    
    cJSON_Delete(root);
    return true;
}

const char* signalk_auth_get_token(void) {
    if (!signalk_auth_is_authenticated()) {
        return NULL;
    }
    return g_auth_context.token;
}

bool signalk_auth_add_token_to_message(char *message_buffer, size_t buffer_size) {
    if (!signalk_auth_is_enabled() || !message_buffer) {
        return true; // No auth needed or invalid params
    }
    
    if (!signalk_auth_is_authenticated()) {
        printf("[SignalK Auth] Cannot add token - not authenticated\n");
        return false;
    }
    
    // Find the closing brace of the JSON message
    size_t msg_len = strlen(message_buffer);
    if (msg_len < 2 || message_buffer[msg_len - 1] != '}') {
        printf("[SignalK Auth] Invalid JSON message format\n");
        return false;
    }
    
    // Calculate space needed for token field
    size_t token_field_len = strlen(",\"token\":\"") + strlen(g_auth_context.token) + strlen("\"");
    
    if (msg_len + token_field_len >= buffer_size) {
        printf("[SignalK Auth] Buffer too small to add token\n");
        return false;
    }
    
    // Insert token before closing brace
    message_buffer[msg_len - 1] = '\0'; // Remove closing brace
    strncat(message_buffer, ",\"token\":\"", buffer_size - strlen(message_buffer) - 1);
    strncat(message_buffer, g_auth_context.token, buffer_size - strlen(message_buffer) - 1);
    strncat(message_buffer, "\"}", buffer_size - strlen(message_buffer) - 1);
    
    return true;
}

size_t signalk_auth_check_token_renewal(char *buffer, size_t buffer_size) {
    if (!signalk_auth_is_enabled() || !buffer) {
        return 0;
    }
    
    // Check if token expires within 5 minutes
    time_t now = time(NULL);
    if (g_auth_context.state == SIGNALK_AUTH_AUTHENTICATED && 
        (g_auth_context.token_expires - now) < 300) {
        
        // Generate token validation/renewal request
        generate_request_id(g_auth_context.request_id, sizeof(g_auth_context.request_id));
        
        int result = snprintf(buffer, buffer_size,
            "{"
            "\"requestId\":\"%s\","
            "\"validate\":{"
            "\"token\":\"%s\""
            "}"
            "}",
            g_auth_context.request_id,
            g_auth_context.token
        );
        
        if (result > 0 && (size_t)result < buffer_size) {
            printf("[SignalK Auth] Requesting token renewal\n");
            return (size_t)result;
        }
    }
    
    return 0;
}

size_t signalk_auth_generate_logout_message(char *buffer, size_t buffer_size) {
    if (!buffer || !signalk_auth_is_enabled()) {
        return 0;
    }
    
    if (g_auth_context.token[0] == '\0') {
        return 0; // No token to logout
    }
    
    generate_request_id(g_auth_context.request_id, sizeof(g_auth_context.request_id));
    
    int result = snprintf(buffer, buffer_size,
        "{"
        "\"requestId\":\"%s\","
        "\"logout\":{"
        "\"token\":\"%s\""
        "}"
        "}",
        g_auth_context.request_id,
        g_auth_context.token
    );
    
    if (result > 0 && (size_t)result < buffer_size) {
        printf("[SignalK Auth] Generated logout message\n");
        return (size_t)result;
    }
    
    return 0;
}

void signalk_auth_reset(void) {
    if (!g_auth_initialized) {
        return;
    }
    
    g_auth_context.state = SIGNALK_AUTH_DISCONNECTED;
    g_auth_context.token[0] = '\0';
    g_auth_context.token_expires = 0;
    printf("[SignalK Auth] Authentication state reset\n");
}

void signalk_auth_cleanup(void) {
    if (!g_auth_initialized) {
        return;
    }
    
    // Clear sensitive data
    memset(&g_auth_context, 0, sizeof(g_auth_context));
    g_auth_initialized = false;
    
    printf("[SignalK Auth] Cleanup completed\n");
}
