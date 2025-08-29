#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "signalk_subscriptions.h"
#include "signalk_hotreload.h"

static int running = 1;

// Sample configuration change callback
void on_config_change(const char* config_file) {
    printf("\n🔄 [APP] Configuration changed! New settings loaded from %s\n", config_file);
    printf("📊 [APP] Active subscriptions: %d\n", signalk_subscription_count);
    
    // Here you could trigger actions like:
    // - Re-establishing WebSocket connections with new settings
    // - Updating subscription patterns
    // - Notifying other system components
    
    printf("✅ [APP] Application successfully adapted to new configuration\n\n");
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[APP] Received signal %d, shutting down gracefully...\n", sig);
        running = 0;
    }
}

int main() {
    printf("🔥 SignalK Hot-Reload Configuration Test\n");
    printf("========================================\n\n");
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    const char* config_file = "settings.json";
    
    // Initial configuration load
    printf("📋 Loading initial configuration from %s...\n", config_file);
    if (!signalk_load_config_from_file(config_file)) {
        printf("❌ Failed to load initial configuration from %s\n", config_file);
        printf("💡 Please ensure the file exists and contains valid JSON\n");
        return 1;
    }
    
    printf("✅ Initial configuration loaded successfully!\n");
    signalk_log_subscription_status();
    
    // Initialize hot-reload system
    printf("\n🔥 Initializing hot-reload system...\n");
    if (!signalk_hotreload_init(config_file, 1000)) {  // Check every 1 second for demo
        printf("❌ Failed to initialize hot-reload system\n");
        return 1;
    }
    
    // Set configuration change callback
    signalk_hotreload_set_callback(on_config_change);
    
    // Start hot-reload service
    if (!signalk_hotreload_start_service()) {
        printf("❌ Failed to start hot-reload service\n");
        return 1;
    }
    
    printf("✅ Hot-reload system active!\n");
    printf("\n📝 INSTRUCTIONS:\n");
    printf("   1. Edit %s while this program is running\n", config_file);
    printf("   2. Add/remove/modify subscription paths or server settings\n");
    printf("   3. Save the file and watch for automatic reload messages\n");
    printf("   4. Press Ctrl+C to exit gracefully\n\n");
    
    printf("👀 Monitoring %s for changes (checking every 1 second)...\n", config_file);
    printf("   Current subscriptions: %d\n\n", signalk_subscription_count);
    
    // Main application loop
    int status_count = 0;
    while (running) {
        sleep(5);  // Simulate application work
        
        // Periodically show status
        status_count++;
        if (status_count % 6 == 0) {  // Every 30 seconds
            printf("⏰ [APP] Status check - subscriptions: %d, hot-reload: %s\n", 
                   signalk_subscription_count, 
                   signalk_hotreload_is_enabled() ? "enabled" : "disabled");
        }
    }
    
    // Cleanup
    printf("\n🧹 Cleaning up hot-reload system...\n");
    signalk_hotreload_stop_service();
    signalk_hotreload_cleanup();
    signalk_free_config();
    
    printf("✅ Hot-reload test completed successfully!\n");
    return 0;
}
