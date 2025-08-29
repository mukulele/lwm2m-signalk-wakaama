#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "signalk_subscriptions.h"

int main() {
    printf("SignalK JSON Configuration Test\n");
    printf("===============================\n\n");
    
    // Test loading configuration from JSON
    printf("Testing configuration loading from settings.json...\n");
    if (signalk_load_config_from_file("settings.json")) {
        printf("✓ Configuration loaded successfully!\n\n");
        
        // Show server configuration
        if (signalk_server_config) {
            printf("Server Configuration:\n");
            printf("  Host: %s\n", signalk_server_config->host);
            printf("  Port: %d\n", signalk_server_config->port);
            printf("  Path: %s\n", signalk_server_config->path);
            printf("  Subscribe Mode: %s\n", signalk_server_config->subscribe_mode);
            printf("\n");
        }
        
        // Show subscription status
        signalk_log_subscription_status();
        
        printf("\nDetailed Subscription Configuration (from JSON):\n");
        for (int i = 0; i < signalk_subscription_count; i++) {
            const signalk_subscription_config_t* sub = &signalk_subscriptions[i];
            printf("  [%2d] %-35s %5d ms (min: %4d ms) %s\n  %*s %s\n", 
                   i + 1, 
                   sub->path, 
                   sub->period_ms, 
                   sub->min_period_ms,
                   sub->high_precision ? "[High Precision]" : "",
                   6, "→", sub->description);
        }
        
        // Test creating subscription message
        printf("\nTesting dynamic subscription message creation...\n");
        char* json_string = NULL;
        if (signalk_create_subscription_message(&json_string)) {
            printf("✓ Successfully created subscription JSON (%zu bytes)\n", strlen(json_string));
            printf("\nFirst 200 characters of generated JSON:\n");
            printf("%.200s...\n", json_string);
            free(json_string);
        } else {
            printf("✗ Failed to create subscription JSON\n");
        }
        
        // Test saving configuration
        printf("\nTesting configuration save...\n");
        if (signalk_save_config_to_file("test_output.json")) {
            printf("✓ Configuration saved to test_output.json\n");
        } else {
            printf("✗ Failed to save configuration\n");
        }
        
        // Clean up
        signalk_free_config();
        
    } else {
        printf("✗ Failed to load configuration from settings.json\n");
        return 1;
    }
    
    printf("\n✓ JSON configuration test completed successfully!\n");
    return 0;
}
