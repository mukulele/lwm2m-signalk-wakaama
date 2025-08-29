#ifndef SIGNALK_CONFIG_INHERITANCE_H
#define SIGNALK_CONFIG_INHERITANCE_H

#include <stdbool.h>
#include <cjson/cJSON.h>

// Environment configuration structure
typedef struct {
    char name[64];                    // Environment name (dev, staging, prod)
    char base_config_file[512];       // Base configuration file path
    char env_config_file[512];        // Environment-specific config file
    char vessel_id[128];              // Optional vessel identifier
    char region[64];                  // Optional region identifier
    bool auto_detect;                 // Auto-detect environment from system
} signalk_env_config_t;

// Configuration inheritance functions
bool signalk_env_init(const char* environment_name);
bool signalk_env_load_config(const char* base_file, const char* env_file);
bool signalk_env_load_with_inheritance(const char* environment);
cJSON* signalk_env_merge_configs(cJSON* base_config, cJSON* env_config);
void signalk_env_cleanup(void);

// Environment detection and management
const char* signalk_env_detect_current(void);
bool signalk_env_set_vessel_id(const char* vessel_id);
bool signalk_env_set_region(const char* region);
void signalk_env_list_available(void);

// Configuration file helpers
bool signalk_env_create_template(const char* environment);
bool signalk_env_validate_inheritance(const char* base_file, const char* env_file);

// Global environment configuration
extern signalk_env_config_t* signalk_env_config;

#endif // SIGNALK_CONFIG_INHERITANCE_H
