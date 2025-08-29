#include "signalk_reconnect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <cjson/cJSON.h>

/**
 * @file signalk_reconnect.c
 * @brief Implementation of automatic reconnection with exponential backoff
 * 
 * Provides robust connection management for marine IoT applications where
 * network connectivity may be intermittent due to environmental factors.
 */

// Global reconnection state
static signalk_reconnect_config_t g_config;
static signalk_connection_state_t g_state;
static bool g_initialized = false;

/**
 * Get current timestamp in milliseconds
 */
static long long get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

/**
 * Generate random number between 0 and max-1
 */
static int random_int(int max) {
    if (max <= 0) return 0;
    return rand() % max;
}

bool signalk_reconnect_init(const signalk_reconnect_config_t *config) {
    if (!config) {
        fprintf(stderr, "SignalK Reconnect: Invalid configuration\n");
        return false;
    }

    // Copy configuration
    memcpy(&g_config, config, sizeof(signalk_reconnect_config_t));

    // Initialize state
    memset(&g_state, 0, sizeof(signalk_connection_state_t));
    g_state.is_connected = false;
    g_state.retry_count = 0;
    g_state.last_attempt = 0;
    g_state.last_success = 0;
    g_state.next_delay_ms = g_config.base_delay_ms;

    // Initialize random seed for jitter
    srand((unsigned int)time(NULL));

    g_initialized = true;
    printf("SignalK Reconnect: Initialized with max_retries=%d, base_delay=%dms, max_delay=%dms\n",
           g_config.max_retries, g_config.base_delay_ms, g_config.max_delay_ms);

    return true;
}

signalk_reconnect_config_t signalk_reconnect_get_default_config(void) {
    signalk_reconnect_config_t config = {
        .auto_reconnect_enabled = true,
        .max_retries = 0,                // Infinite retries for marine IoT
        .base_delay_ms = 1000,           // Start with 1 second
        .max_delay_ms = 300000,          // Cap at 5 minutes
        .backoff_multiplier = 2.0,       // Double delay each attempt
        .jitter_percent = 20,            // 20% jitter to prevent thundering herd
        .connection_timeout_ms = 30000,  // 30 second connection timeout
        .reset_on_success = true         // Reset retry count on success
    };
    return config;
}

bool signalk_reconnect_load_config(const char *config_file) {
    if (!config_file) {
        // Use default configuration
        signalk_reconnect_config_t default_config = signalk_reconnect_get_default_config();
        return signalk_reconnect_init(&default_config);
    }

    FILE *file = fopen(config_file, "r");
    if (!file) {
        printf("SignalK Reconnect: Config file '%s' not found, using defaults\n", config_file);
        signalk_reconnect_config_t default_config = signalk_reconnect_get_default_config();
        return signalk_reconnect_init(&default_config);
    }

    // Read file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *json_string = malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return false;
    }
    
    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);

    // Parse JSON
    cJSON *json = cJSON_Parse(json_string);
    free(json_string);
    
    if (!json) {
        printf("SignalK Reconnect: Invalid JSON in config file, using defaults\n");
        signalk_reconnect_config_t default_config = signalk_reconnect_get_default_config();
        return signalk_reconnect_init(&default_config);
    }

    // Start with defaults and override with config values
    signalk_reconnect_config_t config = signalk_reconnect_get_default_config();

    // Parse reconnection configuration
    cJSON *reconnect = cJSON_GetObjectItem(json, "reconnection");
    if (reconnect) {
        cJSON *item;
        
        item = cJSON_GetObjectItem(reconnect, "auto_reconnect_enabled");
        if (cJSON_IsBool(item)) {
            config.auto_reconnect_enabled = cJSON_IsTrue(item);
        }
        
        item = cJSON_GetObjectItem(reconnect, "max_retries");
        if (cJSON_IsNumber(item)) {
            config.max_retries = item->valueint;
        }
        
        item = cJSON_GetObjectItem(reconnect, "base_delay_ms");
        if (cJSON_IsNumber(item)) {
            config.base_delay_ms = item->valueint;
        }
        
        item = cJSON_GetObjectItem(reconnect, "max_delay_ms");
        if (cJSON_IsNumber(item)) {
            config.max_delay_ms = item->valueint;
        }
        
        item = cJSON_GetObjectItem(reconnect, "backoff_multiplier");
        if (cJSON_IsNumber(item)) {
            config.backoff_multiplier = item->valuedouble;
        }
        
        item = cJSON_GetObjectItem(reconnect, "jitter_percent");
        if (cJSON_IsNumber(item)) {
            config.jitter_percent = item->valueint;
        }
        
        item = cJSON_GetObjectItem(reconnect, "connection_timeout_ms");
        if (cJSON_IsNumber(item)) {
            config.connection_timeout_ms = item->valueint;
        }
        
        item = cJSON_GetObjectItem(reconnect, "reset_on_success");
        if (cJSON_IsBool(item)) {
            config.reset_on_success = cJSON_IsTrue(item);
        }
    }

    cJSON_Delete(json);
    return signalk_reconnect_init(&config);
}

int signalk_reconnect_calculate_delay(int attempt_number) {
    if (!g_initialized || attempt_number <= 0) {
        return g_config.base_delay_ms;
    }

    // Calculate exponential backoff: base_delay * (multiplier ^ (attempt - 1))
    double delay = g_config.base_delay_ms;
    for (int i = 1; i < attempt_number; i++) {
        delay *= g_config.backoff_multiplier;
        if (delay > g_config.max_delay_ms) {
            delay = g_config.max_delay_ms;
            break;
        }
    }

    // Add jitter to prevent thundering herd
    int jitter_range = (int)(delay * g_config.jitter_percent / 100.0);
    int jitter = random_int(jitter_range * 2) - jitter_range; // -jitter_range to +jitter_range
    
    int final_delay = (int)delay + jitter;
    
    // Ensure delay is within bounds
    if (final_delay < g_config.base_delay_ms) {
        final_delay = g_config.base_delay_ms;
    }
    if (final_delay > g_config.max_delay_ms) {
        final_delay = g_config.max_delay_ms;
    }

    return final_delay;
}

signalk_connect_result_t signalk_reconnect_attempt(const char *server, int port) {
    if (!g_initialized) {
        return SIGNALK_CONNECT_FAILED;
    }

    if (!g_config.auto_reconnect_enabled) {
        return SIGNALK_CONNECT_DISABLED;
    }

    // Check if we've exceeded maximum retries
    if (g_config.max_retries > 0 && g_state.retry_count >= g_config.max_retries) {
        snprintf(g_state.last_error, sizeof(g_state.last_error),
                "Maximum retries (%d) exceeded", g_config.max_retries);
        return SIGNALK_CONNECT_MAX_RETRIES;
    }

    // Check if enough time has passed for next retry
    if (!signalk_reconnect_should_retry()) {
        return SIGNALK_CONNECT_FAILED; // Not time yet
    }

    printf("SignalK Reconnect: Attempting connection to %s:%d (attempt %d)\n",
           server, port, g_state.retry_count + 1);

    g_state.last_attempt = time(NULL);
    g_state.retry_count++;

    // Calculate next delay for future attempts
    g_state.next_delay_ms = signalk_reconnect_calculate_delay(g_state.retry_count + 1);

    // TODO: This is where the actual WebSocket connection attempt would happen
    // For now, we'll simulate the connection attempt
    // In the real implementation, this would call lws_client_connect_via_info()
    
    printf("SignalK Reconnect: Connection attempt completed, next delay: %dms\n",
           g_state.next_delay_ms);

    return SIGNALK_CONNECT_SUCCESS; // Placeholder - will be replaced with actual connection logic
}

void signalk_reconnect_on_disconnect(void) {
    if (!g_initialized) return;

    printf("SignalK Reconnect: Connection lost, scheduling reconnection\n");
    g_state.is_connected = false;
    
    // Don't increment retry count here - that happens in attempt function
    // Just mark the time and prepare for reconnection
    g_state.last_attempt = 0; // Allow immediate retry attempt
    
    snprintf(g_state.last_error, sizeof(g_state.last_error),
            "Connection lost, preparing to reconnect");
}

void signalk_reconnect_on_connect(void) {
    if (!g_initialized) return;

    printf("SignalK Reconnect: Connection established successfully\n");
    g_state.is_connected = true;
    g_state.last_success = time(NULL);
    
    if (g_config.reset_on_success) {
        g_state.retry_count = 0;
        g_state.next_delay_ms = g_config.base_delay_ms;
    }
    
    strncpy(g_state.last_error, "Connected successfully", sizeof(g_state.last_error) - 1);
    g_state.last_error[sizeof(g_state.last_error) - 1] = '\0';
}

bool signalk_reconnect_should_retry(void) {
    if (!g_initialized || g_state.is_connected) {
        return false;
    }

    // Check if enough time has passed since last attempt
    time_t now = time(NULL);
    time_t time_since_attempt = now - g_state.last_attempt;
    
    return (time_since_attempt * 1000) >= g_state.next_delay_ms;
}

const signalk_connection_state_t* signalk_reconnect_get_state(void) {
    return g_initialized ? &g_state : NULL;
}

const char* signalk_reconnect_error_string(signalk_connect_result_t result) {
    switch (result) {
        case SIGNALK_CONNECT_SUCCESS:
            return "Connection successful";
        case SIGNALK_CONNECT_FAILED:
            return "Connection failed";
        case SIGNALK_CONNECT_TIMEOUT:
            return "Connection timeout";
        case SIGNALK_CONNECT_MAX_RETRIES:
            return "Maximum retries exceeded";
        case SIGNALK_CONNECT_DISABLED:
            return "Auto-reconnect disabled";
        default:
            return "Unknown error";
    }
}

void signalk_reconnect_reset(void) {
    if (!g_initialized) return;

    printf("SignalK Reconnect: Resetting connection state\n");
    g_state.retry_count = 0;
    g_state.next_delay_ms = g_config.base_delay_ms;
    g_state.last_attempt = 0;
    strncpy(g_state.last_error, "Reset", sizeof(g_state.last_error) - 1);
    g_state.last_error[sizeof(g_state.last_error) - 1] = '\0';
}

bool signalk_reconnect_is_enabled(void) {
    return g_initialized && g_config.auto_reconnect_enabled;
}

void signalk_reconnect_set_enabled(bool enabled) {
    if (g_initialized) {
        g_config.auto_reconnect_enabled = enabled;
        printf("SignalK Reconnect: Auto-reconnect %s\n", enabled ? "enabled" : "disabled");
    }
}

void signalk_reconnect_cleanup(void) {
    if (g_initialized) {
        printf("SignalK Reconnect: Cleaning up\n");
        memset(&g_config, 0, sizeof(g_config));
        memset(&g_state, 0, sizeof(g_state));
        g_initialized = false;
    }
}
