/*******************************************************************************
 * WebSocket Mock Implementation
 * Purpose: Standalone WebSocket mock for isolated unit testing
 * 
 * This mock provides all WebSocket API functions without external dependencies
 * allowing comprehensive testing of WebSocket functionality in isolation.
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include "websocket_mock.h"

/*******************************************************************************
 * Mock WebSocket State Management
 *******************************************************************************/

typedef struct {
    bool connected;
    bool initialized;
    bool network_error;
    char server[256];
    int port;
    char path[256];
    char auth_token[512];
    pthread_mutex_t mutex;
    int message_count;
    char subscriptions[10][256];
    int subscription_count;
} websocket_mock_state_t;

static websocket_mock_state_t mock_state = {0};

/*******************************************************************************
 * Helper Functions
 *******************************************************************************/

static void mock_log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WebSocket Mock] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

static bool is_valid_port(int port) {
    return (port > 0 && port <= 65535);
}

static bool is_valid_server(const char* server) {
    return (server != NULL && strlen(server) > 0 && strlen(server) < 256);
}

static bool is_valid_path(const char* path) {
    return (path != NULL && strlen(path) > 0 && strlen(path) < 256);
}

/*******************************************************************************
 * Mock WebSocket API Implementation
 *******************************************************************************/

int websocket_mock_init(void) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (mock_state.initialized) {
        pthread_mutex_unlock(&mock_state.mutex);
        return 0; // Already initialized
    }
    
    // Initialize mock state
    memset(&mock_state, 0, sizeof(mock_state));
    mock_state.initialized = true;
    mock_state.connected = false;
    mock_state.network_error = false;
    mock_state.message_count = 0;
    mock_state.subscription_count = 0;
    
    // Re-initialize mutex after memset
    pthread_mutex_init(&mock_state.mutex, NULL);
    
    mock_log("WebSocket mock initialized");
    
    pthread_mutex_unlock(&mock_state.mutex);
    return 0;
}

void websocket_mock_cleanup(void) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (!mock_state.initialized) {
        pthread_mutex_unlock(&mock_state.mutex);
        return;
    }
    
    // Disconnect if connected
    if (mock_state.connected) {
        mock_state.connected = false;
        mock_log("Disconnected during cleanup");
    }
    
    // Clear state
    memset(mock_state.server, 0, sizeof(mock_state.server));
    memset(mock_state.path, 0, sizeof(mock_state.path));
    memset(mock_state.auth_token, 0, sizeof(mock_state.auth_token));
    mock_state.port = 0;
    mock_state.message_count = 0;
    mock_state.subscription_count = 0;
    mock_state.initialized = false;
    
    mock_log("WebSocket mock cleaned up");
    
    pthread_mutex_unlock(&mock_state.mutex);
    pthread_mutex_destroy(&mock_state.mutex);
}

bool websocket_mock_is_connected(void) {
    pthread_mutex_lock(&mock_state.mutex);
    bool connected = mock_state.connected;
    pthread_mutex_unlock(&mock_state.mutex);
    return connected;
}

int websocket_mock_connect(const char* server, int port, const char* path) {
    return websocket_mock_connect_with_auth(server, port, path, NULL);
}

int websocket_mock_connect_with_auth(const char* server, int port, const char* path, const char* auth_token) {
    pthread_mutex_lock(&mock_state.mutex);
    
    // Check if already connected
    if (mock_state.connected) {
        mock_log("Warning: Already connected, disconnecting first");
        mock_state.connected = false;
    }
    
    // Simulate network error
    if (mock_state.network_error) {
        mock_log("Error: Network error simulated");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    // Validate parameters
    if (!is_valid_server(server)) {
        mock_log("Error: Invalid server parameter");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    if (!is_valid_port(port)) {
        mock_log("Error: Invalid port parameter");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    if (!is_valid_path(path)) {
        mock_log("Error: Invalid path parameter");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    // Simulate connection delay
    usleep(10000); // 10ms
    
    // Store connection parameters
    strncpy(mock_state.server, server, sizeof(mock_state.server) - 1);
    mock_state.port = port;
    strncpy(mock_state.path, path, sizeof(mock_state.path) - 1);
    
    if (auth_token) {
        strncpy(mock_state.auth_token, auth_token, sizeof(mock_state.auth_token) - 1);
        
        // Simulate authentication validation
        if (strstr(auth_token, "invalid") || strstr(auth_token, "expired")) {
            mock_log("Error: Authentication failed");
            pthread_mutex_unlock(&mock_state.mutex);
            return -2;
        }
    } else {
        memset(mock_state.auth_token, 0, sizeof(mock_state.auth_token));
    }
    
    // Simulate connection failure for unreachable servers
    if (strstr(server, "unreachable") || port == 12345) {
        mock_log("Error: Connection timeout");
        pthread_mutex_unlock(&mock_state.mutex);
        return -3;
    }
    
    mock_state.connected = true;
    mock_log("Connected to %s:%d%s%s", server, port, path, 
             auth_token ? " (authenticated)" : "");
    
    pthread_mutex_unlock(&mock_state.mutex);
    return 0;
}

void websocket_mock_disconnect(void) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (mock_state.connected) {
        mock_state.connected = false;
        mock_log("Disconnected from %s:%d%s", mock_state.server, mock_state.port, mock_state.path);
        
        // Clear subscriptions on disconnect
        mock_state.subscription_count = 0;
        memset(mock_state.subscriptions, 0, sizeof(mock_state.subscriptions));
    }
    
    pthread_mutex_unlock(&mock_state.mutex);
}

int websocket_mock_send_data(const char* data) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (!mock_state.connected) {
        mock_log("Error: Not connected, cannot send data");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    if (!data || strlen(data) == 0) {
        mock_log("Error: Invalid data parameter");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    // Simulate data transmission
    mock_state.message_count++;
    
    // Log abbreviated data for readability
    if (strlen(data) > 100) {
        char abbreviated[101];
        strncpy(abbreviated, data, 100);
        abbreviated[100] = '\0';
        mock_log("Sent message #%d: %.100s...", mock_state.message_count, abbreviated);
    } else {
        mock_log("Sent message #%d: %s", mock_state.message_count, data);
    }
    
    // Simulate transmission delay
    usleep(1000); // 1ms
    
    pthread_mutex_unlock(&mock_state.mutex);
    return 0;
}

int websocket_mock_subscribe(const char* path_pattern) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (!mock_state.connected) {
        mock_log("Error: Not connected, cannot subscribe");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    if (!path_pattern || strlen(path_pattern) == 0) {
        mock_log("Error: Invalid subscription path");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    if (mock_state.subscription_count >= 10) {
        mock_log("Error: Maximum subscriptions reached");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    // Add subscription
    strncpy(mock_state.subscriptions[mock_state.subscription_count], path_pattern, 255);
    mock_state.subscription_count++;
    
    mock_log("Subscribed to: %s (%d/10 subscriptions)", path_pattern, mock_state.subscription_count);
    
    pthread_mutex_unlock(&mock_state.mutex);
    return 0;
}

int websocket_mock_unsubscribe(const char* path_pattern) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (!mock_state.connected) {
        mock_log("Error: Not connected, cannot unsubscribe");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    if (!path_pattern || strlen(path_pattern) == 0) {
        mock_log("Error: Invalid subscription path");
        pthread_mutex_unlock(&mock_state.mutex);
        return -1;
    }
    
    // Find and remove subscription
    for (int i = 0; i < mock_state.subscription_count; i++) {
        if (strcmp(mock_state.subscriptions[i], path_pattern) == 0) {
            // Shift remaining subscriptions
            for (int j = i; j < mock_state.subscription_count - 1; j++) {
                strcpy(mock_state.subscriptions[j], mock_state.subscriptions[j + 1]);
            }
            mock_state.subscription_count--;
            
            mock_log("Unsubscribed from: %s (%d/10 subscriptions)", path_pattern, mock_state.subscription_count);
            pthread_mutex_unlock(&mock_state.mutex);
            return 0;
        }
    }
    
    mock_log("Warning: Subscription not found: %s", path_pattern);
    pthread_mutex_unlock(&mock_state.mutex);
    return -1;
}

void websocket_mock_simulate_network_error(void) {
    pthread_mutex_lock(&mock_state.mutex);
    mock_state.network_error = true;
    mock_log("Network error simulation enabled");
    pthread_mutex_unlock(&mock_state.mutex);
}

void websocket_mock_clear_network_error(void) {
    pthread_mutex_lock(&mock_state.mutex);
    mock_state.network_error = false;
    mock_log("Network error simulation cleared");
    pthread_mutex_unlock(&mock_state.mutex);
}

int websocket_mock_get_message_count(void) {
    pthread_mutex_lock(&mock_state.mutex);
    int count = mock_state.message_count;
    pthread_mutex_unlock(&mock_state.mutex);
    return count;
}

int websocket_mock_get_subscription_count(void) {
    pthread_mutex_lock(&mock_state.mutex);
    int count = mock_state.subscription_count;
    pthread_mutex_unlock(&mock_state.mutex);
    return count;
}

void websocket_mock_get_connection_info(char* server, int* port, char* path) {
    pthread_mutex_lock(&mock_state.mutex);
    
    if (server) {
        strcpy(server, mock_state.server);
    }
    if (port) {
        *port = mock_state.port;
    }
    if (path) {
        strcpy(path, mock_state.path);
    }
    
    pthread_mutex_unlock(&mock_state.mutex);
}
