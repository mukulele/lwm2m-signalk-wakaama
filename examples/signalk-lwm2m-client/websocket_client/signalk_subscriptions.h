#ifndef SIGNALK_SUBSCRIPTIONS_H
#define SIGNALK_SUBSCRIPTIONS_H

#include <cjson/cJSON.h>
#include <stdbool.h>

// Server configuration structure
typedef struct {
    char host[256];
    int port;
    char path[512];
    char subscribe_mode[32];
} signalk_server_config_t;

// Subscription configuration structure
typedef struct {
    char path[256];
    char description[512];
    int period_ms;
    int min_period_ms;
    bool high_precision;
} signalk_subscription_config_t;

// Configuration management functions
bool signalk_load_config_from_file(const char* filename);
bool signalk_save_config_to_file(const char* filename);
bool signalk_create_default_config(void);
void signalk_free_config(void);

// Subscription management functions
bool signalk_create_subscription_message(char** json_string);
bool signalk_process_subscription_response(const char* message);
void signalk_log_subscription_status(void);

// Configuration access
extern signalk_server_config_t* signalk_server_config;
extern signalk_subscription_config_t* signalk_subscriptions;
extern int signalk_subscription_count;

#endif // SIGNALK_SUBSCRIPTIONS_H
