#include <stdio.h>
#include <stdlib.h>
#include "signalk_subscriptions.h"

int main() {
    printf("SignalK Subscription Module Test\n");
    printf("================================\n\n");
    
    // Show subscription status
    signalk_log_subscription_status();
    
    printf("\nDetailed Subscription Configuration:\n");
    for (int i = 0; i < signalk_subscription_count; i++) {
        const signalk_subscription_config_t* sub = &signalk_subscriptions[i];
        printf("  [%2d] %-35s %5d ms (min: %4d ms) %s\n", 
               i + 1, 
               sub->path, 
               sub->period_ms, 
               sub->min_period_ms,
               sub->high_precision ? "[High Precision]" : "");
    }
    
    // Test creating subscription message
    printf("\nTesting subscription message creation...\n");
    char* json_string = NULL;
    if (signalk_create_subscription_message(&json_string)) {
        printf("✓ Successfully created subscription JSON (%zu bytes)\n", strlen(json_string));
        printf("\nGenerated JSON:\n%s\n", json_string);
        free(json_string);
    } else {
        printf("✗ Failed to create subscription JSON\n");
        return 1;
    }
    
    printf("\n✓ Subscription module test completed successfully!\n");
    return 0;
}
