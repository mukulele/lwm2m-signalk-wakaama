// signalK_observe.c
// Implementation for hybrid SignalK-LwM2M observe integration
#include "signalK_observe.h"

#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <libwebsockets.h>

SignalKObservedPath observed_paths[MAX_OBS_PATHS] = {0};

void signalk_observe_start(struct lws *wsi, const char *path) {
    for (int i = 0; i < MAX_OBS_PATHS; ++i) {
        if (!observed_paths[i].active) {
            strncpy(observed_paths[i].path, path, MAX_PATH_LEN - 1);
            observed_paths[i].active = true;
            // Build subscribe message
            cJSON *root = cJSON_CreateObject();
            cJSON *subs = cJSON_CreateArray();
            cJSON *sub = cJSON_CreateObject();
            cJSON_AddStringToObject(sub, "path", path);
            cJSON_AddItemToArray(subs, sub);
            cJSON_AddItemToObject(root, "subscribe", subs);
            char *msg = cJSON_PrintUnformatted(root);
            if (msg) {
                unsigned char buf[LWS_PRE + 512];
                size_t msg_len = strlen(msg);
                if (msg_len < 512) {
                    memcpy(&buf[LWS_PRE], msg, msg_len);
                    lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
                    printf("Sent subscribe: %s\n", msg);
                }
                free(msg);
            }
            cJSON_Delete(root);
            return;
        }
    }
    printf("No free slot for new SignalK observe path!\n");
}

void signalk_observe_stop(struct lws *wsi, const char *path) {
    for (int i = 0; i < MAX_OBS_PATHS; ++i) {
        if (observed_paths[i].active && strcmp(observed_paths[i].path, path) == 0) {
            observed_paths[i].active = false;
            // Build unsubscribe message
            cJSON *root = cJSON_CreateObject();
            cJSON *unsubs = cJSON_CreateArray();
            cJSON *unsub = cJSON_CreateObject();
            cJSON_AddStringToObject(unsub, "path", path);
            cJSON_AddItemToArray(unsubs, unsub);
            cJSON_AddItemToObject(root, "unsubscribe", unsubs);
            char *msg = cJSON_PrintUnformatted(root);
            if (msg) {
                unsigned char buf[LWS_PRE + 512];
                size_t msg_len = strlen(msg);
                if (msg_len < 512) {
                    memcpy(&buf[LWS_PRE], msg, msg_len);
                    lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
                    printf("Sent unsubscribe: %s\n", msg);
                }
                free(msg);
            }
            cJSON_Delete(root);
            return;
        }
    }
}

void signalk_update_value(const char *path, const char *value) {
    for (int i = 0; i < MAX_OBS_PATHS; ++i) {
        if (observed_paths[i].active && strcmp(observed_paths[i].path, path) == 0) {
            strncpy(observed_paths[i].value, value, sizeof(observed_paths[i].value) - 1);
            return;
        }
    }
}

const char *signalk_get_value(const char *path) {
    for (int i = 0; i < MAX_OBS_PATHS; ++i) {
        if (observed_paths[i].active && strcmp(observed_paths[i].path, path) == 0) {
            return observed_paths[i].value;
        }
    }
    return NULL;
}

void signalk_notify_if_changed(const char *path, const char *new_value) {
    for (int i = 0; i < MAX_OBS_PATHS; ++i) {
        if (observed_paths[i].active && strcmp(observed_paths[i].path, path) == 0) {
            if (strcmp(observed_paths[i].value, new_value) != 0) {
                strncpy(observed_paths[i].value, new_value, sizeof(observed_paths[i].value) - 1);
                // TODO: Trigger LwM2M notification for this resource
                printf("Value changed for %s: %s\n", path, new_value);
            }
            return;
        }
    }
}
