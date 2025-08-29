#include "signalk_subscriptions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Global configuration variables
signalk_server_config_t* signalk_server_config = NULL;
signalk_subscription_config_t* signalk_subscriptions = NULL;
int signalk_subscription_count = 0;

// Default configuration file path
static const char* default_config_file = "settings.json";

bool signalk_load_config_from_file(const char* filename) {
    if (!filename) {
        filename = default_config_file;
    }
    
    // Free existing configuration
    signalk_free_config();
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("[SignalK Config] Warning: Could not open %s (errno: %d - %s)\n", 
               filename, errno, strerror(errno));
        printf("[SignalK Config] Will use fallback configuration\n");
        return false;
    }
    
    // Check file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        printf("[SignalK Config] Error: %s is empty or invalid\n", filename);
        fclose(file);
        return false;
    }
    
    if (file_size > 1024 * 1024) { // 1MB limit
        printf("[SignalK Config] Error: %s is too large (%ld bytes, max 1MB)\n", filename, file_size);
        fclose(file);
        return false;
    }
    
    // Read file content
    char* json_string = malloc(file_size + 1);
    if (!json_string) {
        printf("[SignalK Config] Error: Failed to allocate memory for JSON content (%ld bytes)\n", file_size);
        fclose(file);
        return false;
    }
    
    size_t bytes_read = fread(json_string, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        printf("[SignalK Config] Error: Failed to read complete file (read %zu of %ld bytes)\n", 
               bytes_read, file_size);
        free(json_string);
        return false;
    }
    
    json_string[file_size] = '\0';
    
    // Parse JSON
    cJSON* json = cJSON_Parse(json_string);
    const char* json_error = cJSON_GetErrorPtr();
    
    if (!json) {
        printf("[SignalK Config] Error: Invalid JSON in %s\n", filename);
        if (json_error) {
            printf("[SignalK Config] JSON Error near: '%.20s'\n", json_error);
        }
        free(json_string);
        return false;
    }
    
    free(json_string);
    
    // Validate root structure
    if (!cJSON_IsObject(json)) {
        printf("[SignalK Config] Error: Root element must be a JSON object\n");
        cJSON_Delete(json);
        return false;
    }
    
    cJSON* config_root = cJSON_GetObjectItem(json, "signalk_subscriptions");
    if (!config_root) {
        printf("[SignalK Config] Error: Missing 'signalk_subscriptions' root object\n");
        cJSON_Delete(json);
        return false;
    }
    
    if (!cJSON_IsObject(config_root)) {
        printf("[SignalK Config] Error: 'signalk_subscriptions' must be an object\n");
        cJSON_Delete(json);
        return false;
    }
    
    // Load server configuration
    cJSON* server = cJSON_GetObjectItem(config_root, "server");
    if (server) {
        signalk_server_config = malloc(sizeof(signalk_server_config_t));
        if (signalk_server_config) {
            // Set defaults
            strcpy(signalk_server_config->host, "127.0.0.1");
            signalk_server_config->port = 3000;
            strcpy(signalk_server_config->path, "/signalk/v1/stream");
            strcpy(signalk_server_config->subscribe_mode, "none");
            
            // Override with JSON values
            cJSON* host = cJSON_GetObjectItem(server, "host");
            if (cJSON_IsString(host)) {
                strncpy(signalk_server_config->host, cJSON_GetStringValue(host), 255);
                signalk_server_config->host[255] = '\0';
            }
            
            cJSON* port = cJSON_GetObjectItem(server, "port");
            if (cJSON_IsNumber(port)) {
                signalk_server_config->port = cJSON_GetNumberValue(port);
            }
            
            cJSON* path = cJSON_GetObjectItem(server, "path");
            if (cJSON_IsString(path)) {
                strncpy(signalk_server_config->path, cJSON_GetStringValue(path), 511);
                signalk_server_config->path[511] = '\0';
            }
            
            cJSON* subscribe_mode = cJSON_GetObjectItem(server, "subscribe_mode");
            if (cJSON_IsString(subscribe_mode)) {
                strncpy(signalk_server_config->subscribe_mode, cJSON_GetStringValue(subscribe_mode), 31);
                signalk_server_config->subscribe_mode[31] = '\0';
            }
        }
    }
    
    // Load subscriptions
    cJSON* subscriptions = cJSON_GetObjectItem(config_root, "subscriptions");
    if (!subscriptions) {
        printf("[SignalK Config] Warning: No 'subscriptions' array found\n");
        cJSON_Delete(json);
        return true; // Server config might still be valid
    }
    
    if (!cJSON_IsArray(subscriptions)) {
        printf("[SignalK Config] Error: 'subscriptions' must be an array\n");
        cJSON_Delete(json);
        return false;
    }
    
    signalk_subscription_count = cJSON_GetArraySize(subscriptions);
    if (signalk_subscription_count == 0) {
        printf("[SignalK Config] Warning: Empty subscriptions array\n");
        cJSON_Delete(json);
        return true; // Valid but empty
    }
    
    if (signalk_subscription_count > 1000) { // Reasonable limit
        printf("[SignalK Config] Error: Too many subscriptions (%d, max 1000)\n", signalk_subscription_count);
        cJSON_Delete(json);
        return false;
    }
    
    signalk_subscriptions = malloc(signalk_subscription_count * sizeof(signalk_subscription_config_t));
    if (!signalk_subscriptions) {
        printf("[SignalK Config] Error: Failed to allocate memory for %d subscriptions\n", signalk_subscription_count);
        cJSON_Delete(json);
        signalk_subscription_count = 0;
        return false;
    }
    
    // Parse each subscription
    int valid_subscriptions = 0;
    for (int i = 0; i < signalk_subscription_count; i++) {
        cJSON* sub = cJSON_GetArrayItem(subscriptions, i);
        if (!sub || !cJSON_IsObject(sub)) {
            printf("[SignalK Config] Warning: Subscription %d is not a valid object, skipping\n", i);
            continue;
        }
        
        signalk_subscription_config_t* config = &signalk_subscriptions[valid_subscriptions];
        
        // Initialize with defaults
        strcpy(config->path, "");
        strcpy(config->description, "");
        config->period_ms = 5000;
        config->min_period_ms = 2500;
        config->high_precision = false;
        
        // Parse and validate path (required field)
        cJSON* path = cJSON_GetObjectItem(sub, "path");
        if (!cJSON_IsString(path) || strlen(cJSON_GetStringValue(path)) == 0) {
            printf("[SignalK Config] Warning: Subscription %d missing or invalid 'path', skipping\n", i);
            continue;
        }
        
        const char* path_str = cJSON_GetStringValue(path);
        if (strlen(path_str) > 255) {
            printf("[SignalK Config] Warning: Subscription %d path too long (>255 chars), truncating\n", i);
        }
        strncpy(config->path, path_str, 255);
        config->path[255] = '\0';
        
        // Parse optional description
        cJSON* description = cJSON_GetObjectItem(sub, "description");
        if (cJSON_IsString(description)) {
            strncpy(config->description, cJSON_GetStringValue(description), 511);
            config->description[511] = '\0';
        }
        
        // Parse and validate timing
        cJSON* period_ms = cJSON_GetObjectItem(sub, "period_ms");
        if (cJSON_IsNumber(period_ms)) {
            int period_val = cJSON_GetNumberValue(period_ms);
            if (period_val < 100 || period_val > 3600000) { // 100ms to 1 hour
                printf("[SignalK Config] Warning: Subscription %d period_ms out of range (100-3600000), using default\n", i);
            } else {
                config->period_ms = period_val;
            }
        }
        
        cJSON* min_period_ms = cJSON_GetObjectItem(sub, "min_period_ms");
        if (cJSON_IsNumber(min_period_ms)) {
            int min_period_val = cJSON_GetNumberValue(min_period_ms);
            if (min_period_val < 50 || min_period_val > config->period_ms) {
                printf("[SignalK Config] Warning: Subscription %d min_period_ms invalid, using period/2\n", i);
                config->min_period_ms = config->period_ms / 2;
            } else {
                config->min_period_ms = min_period_val;
            }
        }
        
        // Parse boolean flag
        cJSON* high_precision = cJSON_GetObjectItem(sub, "high_precision");
        if (cJSON_IsBool(high_precision)) {
            config->high_precision = cJSON_IsTrue(high_precision);
        }
        
        valid_subscriptions++;
    }
    
    // Update count to reflect only valid subscriptions
    if (valid_subscriptions != signalk_subscription_count) {
        printf("[SignalK Config] Loaded %d valid subscriptions out of %d total\n", 
               valid_subscriptions, signalk_subscription_count);
        signalk_subscription_count = valid_subscriptions;
        
        if (signalk_subscription_count == 0) {
            free(signalk_subscriptions);
            signalk_subscriptions = NULL;
        }
    }
    
    cJSON_Delete(json);
    printf("[SignalK Config] Successfully loaded configuration from %s\n", filename);
    printf("[SignalK Config] Server: %s:%d%s (subscribe=%s)\n", 
           signalk_server_config ? signalk_server_config->host : "unknown",
           signalk_server_config ? signalk_server_config->port : 0,
           signalk_server_config ? signalk_server_config->path : "",
           signalk_server_config ? signalk_server_config->subscribe_mode : "unknown");
    printf("[SignalK Config] Loaded %d subscriptions\n", signalk_subscription_count);
    
    return true;
}

bool signalk_save_config_to_file(const char* filename) {
    if (!filename) {
        filename = default_config_file;
    }
    
    cJSON* root = cJSON_CreateObject();
    cJSON* config_root = cJSON_CreateObject();
    
    if (!root || !config_root) {
        if (root) cJSON_Delete(root);
        if (config_root) cJSON_Delete(config_root);
        return false;
    }
    
    cJSON_AddItemToObject(root, "signalk_subscriptions", config_root);
    
    // Add server configuration
    if (signalk_server_config) {
        cJSON* server = cJSON_CreateObject();
        if (server) {
            cJSON_AddStringToObject(server, "host", signalk_server_config->host);
            cJSON_AddNumberToObject(server, "port", signalk_server_config->port);
            cJSON_AddStringToObject(server, "path", signalk_server_config->path);
            cJSON_AddStringToObject(server, "subscribe_mode", signalk_server_config->subscribe_mode);
            cJSON_AddItemToObject(config_root, "server", server);
        }
    }
    
    // Add subscriptions
    cJSON* subscriptions = cJSON_CreateArray();
    if (subscriptions) {
        for (int i = 0; i < signalk_subscription_count; i++) {
            const signalk_subscription_config_t* config = &signalk_subscriptions[i];
            cJSON* sub = cJSON_CreateObject();
            
            if (sub) {
                cJSON_AddStringToObject(sub, "path", config->path);
                cJSON_AddStringToObject(sub, "description", config->description);
                cJSON_AddNumberToObject(sub, "period_ms", config->period_ms);
                cJSON_AddNumberToObject(sub, "min_period_ms", config->min_period_ms);
                cJSON_AddBoolToObject(sub, "high_precision", config->high_precision);
                cJSON_AddItemToArray(subscriptions, sub);
            }
        }
        cJSON_AddItemToObject(config_root, "subscriptions", subscriptions);
    }
    
    // Write to file
    char* json_string = cJSON_Print(root);
    cJSON_Delete(root);
    
    if (!json_string) {
        return false;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        free(json_string);
        return false;
    }
    
    fprintf(file, "%s", json_string);
    fclose(file);
    free(json_string);
    
    printf("[SignalK Config] Configuration saved to %s\n", filename);
    return true;
}

bool signalk_create_default_config(void) {
    // Free existing configuration
    signalk_free_config();
    
    // Create default server configuration
    signalk_server_config = malloc(sizeof(signalk_server_config_t));
    if (!signalk_server_config) {
        printf("[SignalK Config] Error: Failed to allocate memory for server config\n");
        return false;
    }
    
    strcpy(signalk_server_config->host, "127.0.0.1");
    signalk_server_config->port = 3000;
    strcpy(signalk_server_config->path, "/signalk/v1/stream");
    strcpy(signalk_server_config->subscribe_mode, "none");
    
    // Create minimal default subscriptions
    const char* default_paths[] = {
        "navigation.position",
        "navigation.speedOverGround",
        "navigation.datetime"
    };
    const int default_periods[] = {1000, 1000, 1000};
    const bool default_precision[] = {true, true, false};
    const char* default_descriptions[] = {
        "Vessel position (latitude/longitude)",
        "Speed over ground",
        "Navigation timestamp"
    };
    
    signalk_subscription_count = 3;
    signalk_subscriptions = malloc(signalk_subscription_count * sizeof(signalk_subscription_config_t));
    if (!signalk_subscriptions) {
        printf("[SignalK Config] Error: Failed to allocate memory for default subscriptions\n");
        free(signalk_server_config);
        signalk_server_config = NULL;
        signalk_subscription_count = 0;
        return false;
    }
    
    for (int i = 0; i < signalk_subscription_count; i++) {
        strncpy(signalk_subscriptions[i].path, default_paths[i], 255);
        signalk_subscriptions[i].path[255] = '\0';
        strncpy(signalk_subscriptions[i].description, default_descriptions[i], 511);
        signalk_subscriptions[i].description[511] = '\0';
        signalk_subscriptions[i].period_ms = default_periods[i];
        signalk_subscriptions[i].min_period_ms = default_periods[i] / 2;
        signalk_subscriptions[i].high_precision = default_precision[i];
    }
    
    printf("[SignalK Config] Created minimal default configuration\n");
    return true;
}

void signalk_free_config(void) {
    if (signalk_server_config) {
        free(signalk_server_config);
        signalk_server_config = NULL;
    }
    
    if (signalk_subscriptions) {
        free(signalk_subscriptions);
        signalk_subscriptions = NULL;
    }
    
    signalk_subscription_count = 0;
}

bool signalk_create_subscription_message(char** json_string) {
    if (!json_string || signalk_subscription_count == 0) {
        return false;
    }
    
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        printf("[SignalK Subscription] Error: Failed to create subscription message\n");
        return false;
    }
    
    cJSON *context = cJSON_CreateString("vessels.self");
    cJSON *subscribe = cJSON_CreateArray();
    
    if (!context || !subscribe) {
        cJSON_Delete(json);
        return false;
    }
    
    cJSON_AddItemToObject(json, "context", context);
    cJSON_AddItemToObject(json, "subscribe", subscribe);
    
    // Add each subscription configuration
    for (int i = 0; i < signalk_subscription_count; i++) {
        const signalk_subscription_config_t* sub = &signalk_subscriptions[i];
        
        cJSON *subscription = cJSON_CreateObject();
        if (!subscription) {
            continue;
        }
        
        cJSON *path = cJSON_CreateString(sub->path);
        cJSON *period = cJSON_CreateNumber(sub->period_ms);
        cJSON *minPeriod = cJSON_CreateNumber(sub->min_period_ms);
        cJSON *format = cJSON_CreateString("delta");
        cJSON *policy = cJSON_CreateString("ideal");
        
        if (path && period && minPeriod && format && policy) {
            cJSON_AddItemToObject(subscription, "path", path);
            cJSON_AddItemToObject(subscription, "period", period);
            cJSON_AddItemToObject(subscription, "minPeriod", minPeriod);
            cJSON_AddItemToObject(subscription, "format", format);
            cJSON_AddItemToObject(subscription, "policy", policy);
            
            cJSON_AddItemToArray(subscribe, subscription);
        } else {
            cJSON_Delete(subscription);
        }
    }
    
    *json_string = cJSON_Print(json);
    cJSON_Delete(json);
    
    return (*json_string != NULL);
}

bool signalk_process_subscription_response(const char* message) {
    if (!message) {
        return false;
    }
    
    cJSON *json = cJSON_Parse(message);
    if (!json) {
        return false;
    }
    
    // Check for subscription acknowledgment
    cJSON *requestId = cJSON_GetObjectItem(json, "requestId");
    cJSON *state = cJSON_GetObjectItem(json, "state");
    
    if (requestId && state) {
        if (cJSON_IsString(state)) {
            const char* state_str = cJSON_GetStringValue(state);
            printf("[SignalK Subscription] Request %s: %s\n", 
                   cJSON_GetStringValue(requestId), state_str);
            
            if (strcmp(state_str, "COMPLETED") == 0) {
                printf("[SignalK Subscription] All subscriptions active\n");
                cJSON_Delete(json);
                return true;
            }
        }
    }
    
    // Check for subscription error
    cJSON *errorMessage = cJSON_GetObjectItem(json, "message");
    if (errorMessage && cJSON_IsString(errorMessage)) {
        printf("[SignalK Subscription] Error: %s\n", cJSON_GetStringValue(errorMessage));
    }
    
    cJSON_Delete(json);
    return false;
}

void signalk_log_subscription_status(void) {
    if (signalk_subscription_count == 0) {
        printf("[SignalK Subscription] No configuration loaded. Call signalk_load_config_from_file() first.\n");
        return;
    }
    
    printf("[SignalK Subscription] Configuration:\n");
    printf("  Total subscriptions: %d\n", signalk_subscription_count);
    
    printf("  High-frequency (<=1s): ");
    int high_freq = 0;
    for (int i = 0; i < signalk_subscription_count; i++) {
        if (signalk_subscriptions[i].period_ms <= 1000) {
            high_freq++;
        }
    }
    printf("%d paths\n", high_freq);
    
    printf("  Medium-frequency (2-5s): ");
    int med_freq = 0;
    for (int i = 0; i < signalk_subscription_count; i++) {
        if (signalk_subscriptions[i].period_ms > 1000 && signalk_subscriptions[i].period_ms <= 5000) {
            med_freq++;
        }
    }
    printf("%d paths\n", med_freq);
    
    printf("  Low-frequency (>5s): ");
    int low_freq = 0;
    for (int i = 0; i < signalk_subscription_count; i++) {
        if (signalk_subscriptions[i].period_ms > 5000) {
            low_freq++;
        }
    }
    printf("%d paths\n", low_freq);
}
