// signalK_observe.h
// Data structures and API for hybrid SignalK-LwM2M observe integration
#ifndef SIGNALK_OBSERVE_H
#define SIGNALK_OBSERVE_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_OBS_PATHS 16
#define MAX_PATH_LEN 128

// Structure to hold observed SignalK paths and their latest values
typedef struct {
    char path[MAX_PATH_LEN];
    char value[256]; // Adjust size as needed
    bool active;
} SignalKObservedPath;

// List of currently observed paths
extern SignalKObservedPath observed_paths[MAX_OBS_PATHS];

// API
struct lws;
void signalk_observe_start(struct lws *wsi, const char *path);
void signalk_observe_stop(struct lws *wsi, const char *path);
void signalk_update_value(const char *path, const char *value);
const char *signalk_get_value(const char *path);
void signalk_notify_if_changed(const char *path, const char *new_value);

#endif // SIGNALK_OBSERVE_H
