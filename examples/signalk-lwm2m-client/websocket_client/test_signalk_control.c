#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "signalk_control.h"

/**
 * @file test_signalk_control.c
 * @brief Test program for SignalK PUT command integration
 * 
 * This program tests the SignalK control functionality by sending
 * various PUT commands to control vessel systems.
 */

static volatile int running = 1;

void signal_handler(int sig) {
    printf("\n[Test] Received signal %d, shutting down...\n", sig);
    running = 0;
}

void test_switch_control() {
    printf("\n=== Testing Switch Control ===\n");
    
    // Test navigation lights
    printf("Testing navigation lights...\n");
    signalk_put_result_t result;
    
    result = signalk_control_switch("electrical/switches/navigation/lights", true);
    printf("Navigation lights ON: %s\n", signalk_control_error_string(result));
    sleep(2);
    
    result = signalk_control_switch("electrical/switches/navigation/lights", false);
    printf("Navigation lights OFF: %s\n", signalk_control_error_string(result));
    sleep(1);
    
    // Test bilge pump
    printf("Testing bilge pump...\n");
    result = signalk_control_switch("electrical/switches/bilgePump/main", true);
    printf("Bilge pump ON: %s\n", signalk_control_error_string(result));
    sleep(2);
    
    result = signalk_control_switch("electrical/switches/bilgePump/main", false);
    printf("Bilge pump OFF: %s\n", signalk_control_error_string(result));
}

void test_dimmer_control() {
    printf("\n=== Testing Dimmer Control ===\n");
    
    // Test cabin lights dimmer
    printf("Testing cabin lights dimmer...\n");
    signalk_put_result_t result;
    
    int dimmer_values[] = {25, 50, 75, 100, 50, 0};
    int num_values = sizeof(dimmer_values) / sizeof(dimmer_values[0]);
    
    for (int i = 0; i < num_values && running; i++) {
        result = signalk_control_dimmer("electrical/switches/cabin/lights", dimmer_values[i]);
        printf("Cabin lights dimmer %d%%: %s\n", dimmer_values[i], signalk_control_error_string(result));
        sleep(1);
    }
}

void test_numeric_control() {
    printf("\n=== Testing Numeric Control ===\n");
    
    // Test setting some numeric values
    signalk_put_result_t result;
    
    result = signalk_control_numeric("electrical/batteries/house/voltage", 12.6);
    printf("Battery voltage: %s\n", signalk_control_error_string(result));
    
    result = signalk_control_numeric("navigation/speedOverGround", 5.2);
    printf("Speed over ground: %s\n", signalk_control_error_string(result));
    
    result = signalk_control_numeric("environment/wind/speedOverGround", 8.5);
    printf("Wind speed: %s\n", signalk_control_error_string(result));
}

void test_string_control() {
    printf("\n=== Testing String Control ===\n");
    
    signalk_put_result_t result;
    
    result = signalk_control_string("navigation/gnss/methodQuality", "DGPS");
    printf("GNSS method quality: %s\n", signalk_control_error_string(result));
    
    result = signalk_control_string("design/rig", "Sloop");
    printf("Rig type: %s\n", signalk_control_error_string(result));
}

void print_usage(const char* program_name) {
    printf("SignalK Control Test Program\n");
    printf("Usage: %s [config_file]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  config_file    Path to settings.json file (default: settings.json)\n");
    printf("\n");
    printf("This program tests SignalK PUT command functionality by:\n");
    printf("  - Testing switch control (on/off)\n");
    printf("  - Testing dimmer control (0-100%%)\n");
    printf("  - Testing numeric value control\n");
    printf("  - Testing string value control\n");
    printf("\n");
    printf("Make sure your SignalK server is running and accessible.\n");
}

int main(int argc, char* argv[]) {
    const char* config_file = "settings.json";
    
    if (argc > 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (argc == 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        config_file = argv[1];
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("SignalK Control Test Program\n");
    printf("============================\n");
    printf("Config file: %s\n", config_file);
    printf("Press Ctrl+C to stop\n\n");
    
    // Initialize SignalK control system
    printf("Initializing SignalK control system...\n");
    if (!signalk_control_load_config(config_file)) {
        printf("Error: Failed to load configuration from %s\n", config_file);
        printf("Please ensure the file exists and contains valid SignalK configuration.\n");
        return 1;
    }
    
    // Test connection
    printf("Testing SignalK server connection...\n");
    if (!signalk_control_test_connection()) {
        printf("Warning: SignalK server connection test failed\n");
        printf("Continuing with tests (some may fail)...\n");
    } else {
        printf("✓ SignalK server connection successful\n");
    }
    
    // Run tests
    if (running) test_switch_control();
    if (running) test_dimmer_control();
    if (running) test_numeric_control();
    if (running) test_string_control();
    
    printf("\n=== Test Summary ===\n");
    printf("All SignalK control tests completed.\n");
    printf("Check SignalK server logs for confirmation of received PUT commands.\n");
    
    // Cleanup
    printf("\nCleaning up...\n");
    signalk_control_cleanup();
    
    return 0;
}
