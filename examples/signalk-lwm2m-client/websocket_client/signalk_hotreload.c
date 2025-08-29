#include "signalk_hotreload.h"
#include "signalk_subscriptions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

// Global hot-reload configuration
signalk_hotreload_config_t* signalk_hotreload_config = NULL;
static signalk_config_change_callback_t config_change_callback = NULL;
static pthread_t hotreload_thread;
static bool hotreload_thread_running = false;
static pthread_mutex_t hotreload_mutex = PTHREAD_MUTEX_INITIALIZER;

bool signalk_hotreload_init(const char* config_file, int check_interval_ms) {
    if (!config_file) {
        printf("[HotReload] Error: Config file path is required\n");
        return false;
    }
    
    // Free existing configuration
    signalk_hotreload_cleanup();
    
    // Allocate new configuration
    signalk_hotreload_config = malloc(sizeof(signalk_hotreload_config_t));
    if (!signalk_hotreload_config) {
        printf("[HotReload] Error: Failed to allocate memory for hot-reload config\n");
        return false;
    }
    
    // Initialize configuration
    strncpy(signalk_hotreload_config->config_file_path, config_file, 
            sizeof(signalk_hotreload_config->config_file_path) - 1);
    signalk_hotreload_config->config_file_path[sizeof(signalk_hotreload_config->config_file_path) - 1] = '\0';
    
    signalk_hotreload_config->last_modified = 0;
    signalk_hotreload_config->enabled = true;
    signalk_hotreload_config->check_interval_ms = (check_interval_ms > 0) ? check_interval_ms : 2000; // Default 2s
    signalk_hotreload_config->config_changed = false;
    
    // Get initial file modification time
    struct stat file_stat;
    if (stat(config_file, &file_stat) == 0) {
        signalk_hotreload_config->last_modified = file_stat.st_mtime;
        printf("[HotReload] Initialized with %s (last modified: %ld)\n", 
               config_file, (long)file_stat.st_mtime);
    } else {
        printf("[HotReload] Warning: Could not stat %s: %s\n", config_file, strerror(errno));
    }
    
    return true;
}

void signalk_hotreload_cleanup(void) {
    // Stop the hot-reload thread if running
    if (hotreload_thread_running) {
        hotreload_thread_running = false;
        pthread_join(hotreload_thread, NULL);
    }
    
    // Free configuration
    if (signalk_hotreload_config) {
        free(signalk_hotreload_config);
        signalk_hotreload_config = NULL;
    }
    
    config_change_callback = NULL;
}

bool signalk_hotreload_check_file_change(void) {
    if (!signalk_hotreload_config || !signalk_hotreload_config->enabled) {
        return false;
    }
    
    struct stat file_stat;
    if (stat(signalk_hotreload_config->config_file_path, &file_stat) != 0) {
        // File doesn't exist or can't be accessed
        return false;
    }
    
    // Check if file has been modified
    if (file_stat.st_mtime > signalk_hotreload_config->last_modified) {
        printf("[HotReload] Configuration file modified (old: %ld, new: %ld)\n", 
               (long)signalk_hotreload_config->last_modified, (long)file_stat.st_mtime);
        
        signalk_hotreload_config->last_modified = file_stat.st_mtime;
        signalk_hotreload_config->config_changed = true;
        return true;
    }
    
    return false;
}

void signalk_hotreload_set_callback(signalk_config_change_callback_t callback) {
    config_change_callback = callback;
}

void signalk_hotreload_enable(bool enable) {
    if (signalk_hotreload_config) {
        signalk_hotreload_config->enabled = enable;
        printf("[HotReload] %s\n", enable ? "Enabled" : "Disabled");
    }
}

bool signalk_hotreload_is_enabled(void) {
    return signalk_hotreload_config && signalk_hotreload_config->enabled;
}

// Configuration reload handler
static void handle_config_reload(void) {
    const char* config_file = signalk_hotreload_config->config_file_path;
    
    printf("[HotReload] Reloading configuration from %s...\n", config_file);
    
    // Store current subscription count for comparison
    int old_subscription_count = signalk_subscription_count;
    
    // Attempt to reload configuration
    if (signalk_load_config_from_file(config_file)) {
        printf("[HotReload] ✓ Configuration reloaded successfully!\n");
        printf("[HotReload] Subscriptions: %d → %d\n", old_subscription_count, signalk_subscription_count);
        
        // Log new configuration
        signalk_log_subscription_status();
        
        // Call user-defined callback if set
        if (config_change_callback) {
            config_change_callback(config_file);
        }
        
        printf("[HotReload] Configuration hot-reload completed\n");
    } else {
        printf("[HotReload] ✗ Failed to reload configuration from %s\n", config_file);
        printf("[HotReload] Keeping previous configuration (subscriptions: %d)\n", old_subscription_count);
    }
}

void* signalk_hotreload_service(void* arg) {
    (void)arg; // Unused parameter
    
    printf("[HotReload] Service thread started (check interval: %d ms)\n", 
           signalk_hotreload_config ? signalk_hotreload_config->check_interval_ms : 2000);
    
    while (hotreload_thread_running && signalk_hotreload_config) {
        pthread_mutex_lock(&hotreload_mutex);
        
        if (signalk_hotreload_config->enabled) {
            if (signalk_hotreload_check_file_change()) {
                handle_config_reload();
            }
        }
        
        pthread_mutex_unlock(&hotreload_mutex);
        
        // Sleep for specified interval
        usleep(signalk_hotreload_config->check_interval_ms * 1000);
    }
    
    printf("[HotReload] Service thread stopped\n");
    return NULL;
}

// Start the hot-reload service thread
bool signalk_hotreload_start_service(void) {
    if (!signalk_hotreload_config) {
        printf("[HotReload] Error: Hot-reload not initialized\n");
        return false;
    }
    
    if (hotreload_thread_running) {
        printf("[HotReload] Service already running\n");
        return true;
    }
    
    hotreload_thread_running = true;
    
    if (pthread_create(&hotreload_thread, NULL, signalk_hotreload_service, NULL) != 0) {
        printf("[HotReload] Error: Failed to create hot-reload service thread\n");
        hotreload_thread_running = false;
        return false;
    }
    
    printf("[HotReload] Service started successfully\n");
    return true;
}

// Stop the hot-reload service thread
void signalk_hotreload_stop_service(void) {
    if (hotreload_thread_running) {
        printf("[HotReload] Stopping service...\n");
        hotreload_thread_running = false;
        pthread_join(hotreload_thread, NULL);
        printf("[HotReload] Service stopped\n");
    }
}
