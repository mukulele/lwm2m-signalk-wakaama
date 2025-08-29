#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include "signalk_auth.h"

// Structure to hold HTTP response data
struct auth_response {
    char *data;
    size_t size;
};

// Callback function to write HTTP response data
static size_t write_callback(void *contents, size_t size, size_t nmemb, struct auth_response *response) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(response->data, response->size + realsize + 1);
    
    if (!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0;
    
    return realsize;
}

// Test HTTP-based authentication (like your curl example)
int test_http_auth(const char *username, const char *password) {
    CURL *curl;
    CURLcode res;
    struct auth_response response = {0};
    char post_data[256];
    
    printf("\n=== Testing HTTP Authentication ===\n");
    
    // Prepare POST data
    snprintf(post_data, sizeof(post_data), 
             "{\"username\":\"%s\",\"password\":\"%s\"}", 
             username, password);
    
    curl = curl_easy_init();
    if (!curl) {
        printf("Failed to initialize curl\n");
        return 0;
    }
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:3000/signalk/v1/auth/login");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Perform the request
    printf("Sending login request to SignalK server...\n");
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 0;
    }
    
    // Check HTTP response code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    printf("HTTP Response Code: %ld\n", response_code);
    
    if (response_code == 200 && response.data) {
        printf("Authentication Response: %s\n", response.data);
        printf("✅ HTTP Authentication successful!\n");
    } else {
        printf("❌ HTTP Authentication failed\n");
    }
    
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (response.data) free(response.data);
    
    return (response_code == 200);
}

// Test the auth module with a simulated SignalK response
int test_auth_module() {
    printf("\n=== Testing Authentication Module ===\n");
    
    // Initialize auth module
    signalk_auth_config_t config = {
        .enabled = true,
        .username = "pi",
        .password = "clipperiv",
        .token_renewal_time = 3600
    };
    
    if (!signalk_auth_init(&config)) {
        printf("❌ Failed to initialize auth module\n");
        return 0;
    }
    
    printf("✅ Auth module initialized\n");
    printf("Auth enabled: %s\n", signalk_auth_is_enabled() ? "YES" : "NO");
    printf("Auth state: %d\n", signalk_auth_get_state());
    
    // Test login message generation
    char login_msg[512];
    size_t login_len = signalk_auth_generate_login_message(login_msg, sizeof(login_msg));
    
    if (login_len > 0) {
        printf("✅ Generated login message (%zu bytes):\n%s\n", login_len, login_msg);
    } else {
        printf("❌ Failed to generate login message\n");
        return 0;
    }
    
    // Extract the request ID from the generated login message
    char request_id[64] = {0};
    char *id_start = strstr(login_msg, "\"requestId\":\"");
    if (id_start) {
        id_start += strlen("\"requestId\":\"");
        char *id_end = strchr(id_start, '"');
        if (id_end) {
            size_t id_len = id_end - id_start;
            if (id_len < sizeof(request_id)) {
                strncpy(request_id, id_start, id_len);
                request_id[id_len] = '\0';
            }
        }
    }
    
    // Create mock response with the correct request ID
    char mock_response[512];
    snprintf(mock_response, sizeof(mock_response),
        "{"
        "\"requestId\":\"%s\","
        "\"result\":200,"
        "\"login\":{"
        "\"token\":\"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6InBpIiwiaWF0IjoxNzU2NDkwNzA1fQ.nBrBKYYpihdTt3HkwAWL17L1BVAOV2gzjuOidzE-TAs\","
        "\"timeToLive\":3600"
        "}"
        "}", request_id);
    
    printf("\nSimulating SignalK auth response with request ID: %s\n", request_id);
    if (signalk_auth_process_response(mock_response)) {
        printf("✅ Authentication response processed successfully\n");
        printf("Auth state: %d\n", signalk_auth_get_state());
        printf("Is authenticated: %s\n", signalk_auth_is_authenticated() ? "YES" : "NO");
        
        const char *token = signalk_auth_get_token();
        if (token) {
            printf("✅ Token retrieved: %.50s...\n", token);
        }
    } else {
        printf("❌ Failed to process authentication response\n");
        return 0;
    }
    
    // Test adding token to message
    char test_msg[512] = "{\"put\":{\"path\":\"electrical.switches.cabin.lights\",\"value\":true}}";
    printf("\nTesting token addition to message...\n");
    printf("Original message: %s\n", test_msg);
    
    if (signalk_auth_add_token_to_message(test_msg, sizeof(test_msg))) {
        printf("✅ Token added to message:\n%s\n", test_msg);
    } else {
        printf("❌ Failed to add token to message\n");
    }
    
    // Test logout message
    char logout_msg[256];
    size_t logout_len = signalk_auth_generate_logout_message(logout_msg, sizeof(logout_msg));
    if (logout_len > 0) {
        printf("✅ Generated logout message (%zu bytes):\n%s\n", logout_len, logout_msg);
    }
    
    // Cleanup
    signalk_auth_cleanup();
    printf("✅ Auth module cleanup completed\n");
    
    return 1;
}

int main() {
    printf("SignalK Authentication Module Test\n");
    printf("==================================\n");
    
    // Test 1: HTTP authentication
    int http_success = test_http_auth("pi", "clipperiv");
    
    // Test 2: Auth module functionality
    int module_success = test_auth_module();
    
    printf("\n=== Test Results ===\n");
    printf("HTTP Authentication: %s\n", http_success ? "✅ PASS" : "❌ FAIL");
    printf("Auth Module: %s\n", module_success ? "✅ PASS" : "❌ FAIL");
    
    if (http_success && module_success) {
        printf("\n🎉 All authentication tests passed!\n");
        printf("Your auth module is ready for integration.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Check the output above.\n");
        return 1;
    }
}
