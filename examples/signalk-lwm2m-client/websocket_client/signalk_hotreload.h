#ifndef SIGNALK_HOTRELOAD_H
#define SIGNALK_HOTRELOAD_H

#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>

// Hot-reload configuration structure
typedef struct {
    char config_file_path[512];
    time_t last_modified;
    bool enabled;
    int check_interval_ms;
    bool config_changed;
} signalk_hotreload_config_t;

// Callback function type for configuration changes
typedef void (*signalk_config_change_callback_t)(const char* config_file);

// Hot-reload management functions
bool signalk_hotreload_init(const char* config_file, int check_interval_ms);
void signalk_hotreload_cleanup(void);
bool signalk_hotreload_check_file_change(void);
void signalk_hotreload_set_callback(signalk_config_change_callback_t callback);
void signalk_hotreload_enable(bool enable);
bool signalk_hotreload_is_enabled(void);

// Thread-safe hot-reload service
void* signalk_hotreload_service(void* arg);
bool signalk_hotreload_start_service(void);
void signalk_hotreload_stop_service(void);

// Global hot-reload configuration
extern signalk_hotreload_config_t* signalk_hotreload_config;

#endif // SIGNALK_HOTRELOAD_H
