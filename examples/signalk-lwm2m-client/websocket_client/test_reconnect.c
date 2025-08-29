#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "signalk_reconnect.h"

/**
 * @file test_reconnect.c
 * @brief Test program for SignalK automatic reconnection system
 * 
 * This program demonstrates the exponential backoff reconnection logic
 * and allows testing of various configuration scenarios.
 */

static int running = 1;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
}

void print_state_info(void) {
    const signalk_connection_state_t *state = signalk_reconnect_get_state();
    if (state) {
        printf("Connection State:\n");
        printf("  Connected: %s\n", state->is_connected ? "YES" : "NO");
        printf("  Retry Count: %d\n", state->retry_count);
        printf("  Next Delay: %dms\n", state->next_delay_ms);
        printf("  Last Error: %s\n", state->last_error);
        printf("  Auto-reconnect: %s\n", signalk_reconnect_is_enabled() ? "ENABLED" : "DISABLED");
        printf("\n");
    }
}

void test_exponential_backoff(void) {
    printf("=== Testing Exponential Backoff Calculation ===\n");
    
    for (int attempt = 1; attempt <= 10; attempt++) {
        int delay = signalk_reconnect_calculate_delay(attempt);
        printf("Attempt %2d: %6dms (%3.1fs)\n", attempt, delay, delay / 1000.0);
    }
    printf("\n");
}

void test_reconnection_config(void) {
    printf("=== Testing Different Configurations ===\n");
    
    // Test 1: Default marine IoT config
    printf("1. Marine IoT Default Configuration:\n");
    signalk_reconnect_config_t marine_config = signalk_reconnect_get_default_config();
    printf("   Base delay: %dms, Max delay: %dms, Multiplier: %.1f\n",
           marine_config.base_delay_ms, marine_config.max_delay_ms, marine_config.backoff_multiplier);
    printf("   Max retries: %d (infinite), Jitter: %d%%\n",
           marine_config.max_retries, marine_config.jitter_percent);
    
    // Test 2: Fast reconnection for testing
    printf("\n2. Fast Reconnection Configuration:\n");
    signalk_reconnect_config_t fast_config = {
        .auto_reconnect_enabled = true,
        .max_retries = 5,
        .base_delay_ms = 100,
        .max_delay_ms = 2000,
        .backoff_multiplier = 1.5,
        .jitter_percent = 10,
        .connection_timeout_ms = 5000,
        .reset_on_success = true
    };
    printf("   Base delay: %dms, Max delay: %dms, Multiplier: %.1f\n",
           fast_config.base_delay_ms, fast_config.max_delay_ms, fast_config.backoff_multiplier);
    printf("   Max retries: %d, Jitter: %d%%\n",
           fast_config.max_retries, fast_config.jitter_percent);
    
    // Test 3: Conservative config for unstable networks
    printf("\n3. Conservative Configuration:\n");
    signalk_reconnect_config_t conservative_config = {
        .auto_reconnect_enabled = true,
        .max_retries = 0,
        .base_delay_ms = 5000,
        .max_delay_ms = 600000,  // 10 minutes
        .backoff_multiplier = 2.5,
        .jitter_percent = 30,
        .connection_timeout_ms = 60000,
        .reset_on_success = true
    };
    printf("   Base delay: %dms, Max delay: %dms, Multiplier: %.1f\n",
           conservative_config.base_delay_ms, conservative_config.max_delay_ms, conservative_config.backoff_multiplier);
    printf("   Max retries: %d (infinite), Jitter: %d%%\n",
           conservative_config.max_retries, conservative_config.jitter_percent);
    
    printf("\n");
}

void simulate_connection_scenario(const char *scenario_name, int disconnect_count) {
    printf("=== Simulating: %s ===\n", scenario_name);
    
    for (int i = 1; i <= disconnect_count; i++) {
        printf("Disconnect #%d:\n", i);
        
        // Simulate connection loss
        signalk_reconnect_on_disconnect();
        print_state_info();
        
        // Simulate reconnection attempts
        for (int attempt = 1; attempt <= 3; attempt++) {
            if (signalk_reconnect_should_retry()) {
                printf("  Attempting reconnection (try %d)...\n", attempt);
                signalk_connect_result_t result = signalk_reconnect_attempt("demo.signalk.org", 3000);
                printf("  Result: %s\n", signalk_reconnect_error_string(result));
                
                // Simulate success on 3rd attempt
                if (attempt == 3) {
                    signalk_reconnect_on_connect();
                    printf("  ✓ Connection restored!\n");
                    break;
                }
            } else {
                printf("  Waiting for retry interval...\n");
            }
            
            // Short delay for simulation
            usleep(100000); // 100ms
        }
        
        print_state_info();
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    printf("SignalK Reconnection System Test\n");
    printf("================================\n\n");
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Test configuration options
    test_reconnection_config();
    
    // Test exponential backoff calculation
    test_exponential_backoff();
    
    // Initialize with default configuration
    printf("Initializing reconnection system...\n");
    if (!signalk_reconnect_load_config("settings.json")) {
        printf("Using default configuration\n");
    }
    printf("\n");
    
    print_state_info();
    
    // Run interactive tests if no arguments provided
    if (argc == 1) {
        printf("Available test scenarios:\n");
        printf("  1. Short disconnection burst (3 quick disconnects)\n");
        printf("  2. Extended outage simulation (5 disconnects with increasing delays)\n");
        printf("  3. Stress test (10 rapid disconnects)\n");
        printf("  4. Manual control (interactive)\n");
        printf("\nEnter test number (1-4) or 'q' to quit: ");
        
        char input[10];
        if (fgets(input, sizeof(input), stdin)) {
            switch (input[0]) {
                case '1':
                    simulate_connection_scenario("Short Disconnection Burst", 3);
                    break;
                case '2':
                    simulate_connection_scenario("Extended Outage Simulation", 5);
                    break;
                case '3':
                    simulate_connection_scenario("Stress Test", 10);
                    break;
                case '4':
                    printf("\nManual control mode - use Ctrl+C to exit\n");
                    printf("Commands: 'd' = disconnect, 'c' = connect, 'r' = reset, 's' = status\n");
                    
                    while (running) {
                        printf("> ");
                        if (fgets(input, sizeof(input), stdin)) {
                            switch (input[0]) {
                                case 'd':
                                    signalk_reconnect_on_disconnect();
                                    printf("Simulated disconnect\n");
                                    break;
                                case 'c':
                                    signalk_reconnect_on_connect();
                                    printf("Simulated connect\n");
                                    break;
                                case 'r':
                                    signalk_reconnect_reset();
                                    printf("Reset state\n");
                                    break;
                                case 's':
                                    print_state_info();
                                    break;
                                case 'q':
                                    running = 0;
                                    break;
                                default:
                                    printf("Unknown command\n");
                                    break;
                            }
                        }
                    }
                    break;
                default:
                    printf("Invalid option\n");
                    break;
            }
        }
    } else {
        // Command line test mode
        if (strcmp(argv[1], "burst") == 0) {
            simulate_connection_scenario("Command Line Burst Test", 3);
        } else if (strcmp(argv[1], "extended") == 0) {
            simulate_connection_scenario("Command Line Extended Test", 5);
        } else if (strcmp(argv[1], "stress") == 0) {
            simulate_connection_scenario("Command Line Stress Test", 10);
        } else {
            printf("Usage: %s [burst|extended|stress]\n", argv[0]);
            printf("   or run without arguments for interactive mode\n");
        }
    }
    
    // Cleanup
    printf("\nCleaning up...\n");
    signalk_reconnect_cleanup();
    
    printf("Test completed.\n");
    return 0;
}
