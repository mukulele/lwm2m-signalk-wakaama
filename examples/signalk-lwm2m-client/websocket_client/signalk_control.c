#include "signalk_control.h"
#include "signalk_subscriptions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

/**
 * @file signalk_control.c
 * @brief SignalK PUT command integration implementation
 * 
 * This module enables LwM2M servers to control vessel systems by sending
 * HTTP PUT commands to SignalK servers. It provides a robust interface
 * for bidirectional communication in marine IoT scenarios.
 */

static signalk_control_config_t control_config;
static bool config_initialized = false;
static CURL *curl_handle = NULL;

/**
 * Response data structure for HTTP requests
 */
typedef struct {
    char *data;
    size_t size;
} http_response_t;

/**
 * Callback function to handle HTTP response data
 */
static size_t write_response_callback(void *contents, size_t size, size_t nmemb, http_response_t *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->data, response->size + realsize + 1);
    
    if (!ptr) {
        printf("[SignalK Control] Memory allocation failed for HTTP response\n");
        return 0;
    }
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0; // Null terminate
    
    return realsize;
}

bool signalk_control_init(const signalk_control_config_t *config) {
    if (!config) {
        printf("[SignalK Control] Error: NULL configuration provided\n");
        return false;
    }
    
    // Copy configuration
    memcpy(&control_config, config, sizeof(signalk_control_config_t));
    
    // Initialize libcurl
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        printf("[SignalK Control] Error: Failed to initialize libcurl\n");
        return false;
    }
    
    // Create a reusable curl handle
    curl_handle = curl_easy_init();
    if (!curl_handle) {
        printf("[SignalK Control] Error: Failed to create curl handle\n");
        curl_global_cleanup();
        return false;
    }
    
    // Set common curl options
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, control_config.timeout_ms / 1000);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_response_callback);
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    
    if (!control_config.verify_ssl) {
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    
    config_initialized = true;
    
    printf("[SignalK Control] ✓ Initialized - Server: %s:%d, Vessel: %s\n", 
           control_config.server_host, control_config.server_port, control_config.vessel_id);
    
    return true;
}

bool signalk_control_load_config(const char *config_file) {
    // Load the configuration using existing infrastructure
    if (!signalk_load_config_from_file(config_file)) {
        return false;
    }
    
    // Default configuration
    signalk_control_config_t config = {
        .server_host = "127.0.0.1",
        .server_port = 3000,
        .vessel_id = "self",
        .timeout_ms = 5000,
        .verify_ssl = false
    };
    
    // Use the loaded SignalK server configuration
    extern signalk_server_config_t* signalk_server_config;
    if (signalk_server_config) {
        strncpy(config.server_host, signalk_server_config->host, sizeof(config.server_host) - 1);
        config.server_port = signalk_server_config->port;
    }
    
    // Note: For now, we use default control configuration
    // In the future, this could be extended to load control-specific settings
    // from the JSON configuration file
    
    return signalk_control_init(&config);
}

static signalk_put_result_t send_put_request(const char *path, const char *json_value) {
    if (!config_initialized || !curl_handle) {
        printf("[SignalK Control] Error: Not initialized\n");
        return SIGNALK_PUT_ERROR_CONFIG;
    }
    
    // Build URL: http://host:port/signalk/v1/api/vessels/self/path
    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/signalk/v1/api/vessels/%s/%s", 
             control_config.server_host, control_config.server_port, 
             control_config.vessel_id, path);
    
    // Prepare HTTP response buffer
    http_response_t response = {0};
    
    // Set curl options for this request
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, json_value);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);
    
    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    
    printf("[SignalK Control] PUT %s -> %s\n", path, json_value);
    
    // Perform the request
    CURLcode curl_result = curl_easy_perform(curl_handle);
    
    // Cleanup headers
    curl_slist_free_all(headers);
    
    signalk_put_result_t result = SIGNALK_PUT_SUCCESS;
    
    if (curl_result != CURLE_OK) {
        printf("[SignalK Control] Error: %s\n", curl_easy_strerror(curl_result));
        
        switch (curl_result) {
            case CURLE_OPERATION_TIMEDOUT:
                result = SIGNALK_PUT_ERROR_TIMEOUT;
                break;
            case CURLE_COULDNT_CONNECT:
            case CURLE_COULDNT_RESOLVE_HOST:
                result = SIGNALK_PUT_ERROR_NETWORK;
                break;
            default:
                result = SIGNALK_PUT_ERROR_HTTP;
                break;
        }
    } else {
        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code >= 200 && response_code < 300) {
            printf("[SignalK Control] ✓ PUT successful (HTTP %ld)\n", response_code);
            result = SIGNALK_PUT_SUCCESS;
        } else {
            printf("[SignalK Control] ✗ PUT failed (HTTP %ld)\n", response_code);
            result = SIGNALK_PUT_ERROR_HTTP;
        }
    }
    
    // Clean up response data
    if (response.data) {
        free(response.data);
    }
    
    return result;
}

signalk_put_result_t signalk_control_switch(const char *switch_path, bool state) {
    if (!switch_path) {
        return SIGNALK_PUT_ERROR_CONFIG;
    }
    
    // Create JSON value: {"value": true/false}
    cJSON *json = cJSON_CreateObject();
    cJSON *value = cJSON_CreateBool(state);
    cJSON_AddItemToObject(json, "value", value);
    
    char *json_string = cJSON_Print(json);
    signalk_put_result_t result = send_put_request(switch_path, json_string);
    
    printf("[SignalK Control] Switch %s -> %s\n", switch_path, state ? "ON" : "OFF");
    
    free(json_string);
    cJSON_Delete(json);
    
    return result;
}

signalk_put_result_t signalk_control_dimmer(const char *dimmer_path, int dimmer_value) {
    if (!dimmer_path || dimmer_value < 0 || dimmer_value > 100) {
        return SIGNALK_PUT_ERROR_CONFIG;
    }
    
    // Create JSON value: {"value": dimmer_value}
    cJSON *json = cJSON_CreateObject();
    cJSON *value = cJSON_CreateNumber(dimmer_value);
    cJSON_AddItemToObject(json, "value", value);
    
    char *json_string = cJSON_Print(json);
    signalk_put_result_t result = send_put_request(dimmer_path, json_string);
    
    printf("[SignalK Control] Dimmer %s -> %d%%\n", dimmer_path, dimmer_value);
    
    free(json_string);
    cJSON_Delete(json);
    
    return result;
}

signalk_put_result_t signalk_control_numeric(const char *path, double value) {
    if (!path) {
        return SIGNALK_PUT_ERROR_CONFIG;
    }
    
    // Create JSON value: {"value": numeric_value}
    cJSON *json = cJSON_CreateObject();
    cJSON *json_value = cJSON_CreateNumber(value);
    cJSON_AddItemToObject(json, "value", json_value);
    
    char *json_string = cJSON_Print(json);
    signalk_put_result_t result = send_put_request(path, json_string);
    
    printf("[SignalK Control] Numeric %s -> %.3f\n", path, value);
    
    free(json_string);
    cJSON_Delete(json);
    
    return result;
}

signalk_put_result_t signalk_control_string(const char *path, const char *value) {
    if (!path || !value) {
        return SIGNALK_PUT_ERROR_CONFIG;
    }
    
    // Create JSON value: {"value": "string_value"}
    cJSON *json = cJSON_CreateObject();
    cJSON *json_value = cJSON_CreateString(value);
    cJSON_AddItemToObject(json, "value", json_value);
    
    char *json_string = cJSON_Print(json);
    signalk_put_result_t result = send_put_request(path, json_string);
    
    printf("[SignalK Control] String %s -> \"%s\"\n", path, value);
    
    free(json_string);
    cJSON_Delete(json);
    
    return result;
}

const char *signalk_control_error_string(signalk_put_result_t result) {
    switch (result) {
        case SIGNALK_PUT_SUCCESS:
            return "Success";
        case SIGNALK_PUT_ERROR_NETWORK:
            return "Network/connection error";
        case SIGNALK_PUT_ERROR_HTTP:
            return "HTTP error response";
        case SIGNALK_PUT_ERROR_JSON:
            return "JSON formatting error";
        case SIGNALK_PUT_ERROR_TIMEOUT:
            return "Request timeout";
        case SIGNALK_PUT_ERROR_CONFIG:
            return "Configuration error";
        default:
            return "Unknown error";
    }
}

bool signalk_control_test_connection(void) {
    if (!config_initialized) {
        return false;
    }
    
    // Test with a simple GET request to SignalK API
    char url[1024];
    snprintf(url, sizeof(url), "http://%s:%d/signalk/v1/api/vessels/%s", 
             control_config.server_host, control_config.server_port, control_config.vessel_id);
    
    http_response_t response = {0};
    
    CURL *test_handle = curl_easy_init();
    if (!test_handle) {
        return false;
    }
    
    curl_easy_setopt(test_handle, CURLOPT_URL, url);
    curl_easy_setopt(test_handle, CURLOPT_WRITEFUNCTION, write_response_callback);
    curl_easy_setopt(test_handle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(test_handle, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(test_handle, CURLOPT_FAILONERROR, 1L);
    
    CURLcode result = curl_easy_perform(test_handle);
    bool success = (result == CURLE_OK);
    
    if (success) {
        printf("[SignalK Control] ✓ Connection test successful\n");
    } else {
        printf("[SignalK Control] ✗ Connection test failed: %s\n", curl_easy_strerror(result));
    }
    
    curl_easy_cleanup(test_handle);
    if (response.data) {
        free(response.data);
    }
    
    return success;
}

void signalk_control_cleanup(void) {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
        curl_handle = NULL;
    }
    
    curl_global_cleanup();
    config_initialized = false;
    
    printf("[SignalK Control] Cleanup completed\n");
}
