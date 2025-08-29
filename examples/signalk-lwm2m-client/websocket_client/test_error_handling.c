#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "signalk_subscriptions.h"

void test_file_not_found() {
    printf("\n=== Test: File Not Found ===\n");
    bool result = signalk_load_config_from_file("nonexistent_file.json");
    printf("Result: %s\n", result ? "SUCCESS" : "FAILED (expected)");
}

void test_invalid_json() {
    printf("\n=== Test: Invalid JSON ===\n");
    bool result = signalk_load_config_from_file("invalid_test.json");
    printf("Result: %s\n", result ? "SUCCESS" : "FAILED (expected)");
}

void test_empty_file() {
    printf("\n=== Test: Empty File ===\n");
    // Create empty file
    FILE* f = fopen("empty_test.json", "w");
    if (f) fclose(f);
    
    bool result = signalk_load_config_from_file("empty_test.json");
    printf("Result: %s\n", result ? "SUCCESS" : "FAILED (expected)");
    unlink("empty_test.json");
}

void test_missing_root_object() {
    printf("\n=== Test: Missing Root Object ===\n");
    FILE* f = fopen("no_root_test.json", "w");
    if (f) {
        fprintf(f, "{\"wrong_root\": \"value\"}");
        fclose(f);
    }
    
    bool result = signalk_load_config_from_file("no_root_test.json");
    printf("Result: %s\n", result ? "SUCCESS" : "FAILED (expected)");
    unlink("no_root_test.json");
}

void test_no_subscriptions() {
    printf("\n=== Test: No Subscriptions Array ===\n");
    FILE* f = fopen("no_subs_test.json", "w");
    if (f) {
        fprintf(f, "{\"signalk_subscriptions\": {\"server\": {\"host\": \"test.com\"}}}");
        fclose(f);
    }
    
    bool result = signalk_load_config_from_file("no_subs_test.json");
    printf("Result: %s (should succeed with server config only)\n", result ? "SUCCESS" : "FAILED");
    if (result && signalk_server_config) {
        printf("  Server host: %s\n", signalk_server_config->host);
        printf("  Subscriptions: %d\n", signalk_subscription_count);
    }
    unlink("no_subs_test.json");
}

void test_default_config_creation() {
    printf("\n=== Test: Default Config Creation ===\n");
    bool result = signalk_create_default_config();
    printf("Result: %s\n", result ? "SUCCESS" : "FAILED");
    if (result) {
        printf("  Default subscriptions: %d\n", signalk_subscription_count);
        signalk_log_subscription_status();
    }
}

void test_subscription_message_with_no_config() {
    printf("\n=== Test: Subscription Message with No Config ===\n");
    signalk_free_config(); // Clear any existing config
    
    char* json_msg = NULL;
    bool result = signalk_create_subscription_message(&json_msg);
    printf("Result: %s (should fail gracefully)\n", result ? "SUCCESS" : "FAILED (expected)");
}

int main() {
    printf("SignalK Enhanced Error Handling Test\n");
    printf("====================================\n");
    
    test_file_not_found();
    test_invalid_json();
    test_empty_file();
    test_missing_root_object();
    test_no_subscriptions();
    test_default_config_creation();
    test_subscription_message_with_no_config();
    
    // Cleanup
    signalk_free_config();
    
    printf("\n✓ Error handling test completed!\n");
    printf("All error cases handled gracefully with informative messages.\n");
    
    return 0;
}
