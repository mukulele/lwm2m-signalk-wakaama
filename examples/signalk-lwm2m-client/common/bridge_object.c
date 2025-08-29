#include "bridge_object.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define MAX_BRIDGE_RESOURCES 128

static bridge_resource_t registry[MAX_BRIDGE_RESOURCES];
static int registry_count = 0;
static pthread_mutex_t reg_mutex = PTHREAD_MUTEX_INITIALIZER;

void bridge_init(void) {
    pthread_mutex_lock(&reg_mutex);
    registry_count = 0;
    memset(registry, 0, sizeof(registry));
    pthread_mutex_unlock(&reg_mutex);
}

int bridge_register(uint16_t objId, uint16_t instId, uint16_t resId, const char *signalK_path) {
    pthread_mutex_lock(&reg_mutex);

    if (registry_count >= MAX_BRIDGE_RESOURCES) {
        printf("[Bridge] Warning: Registry full (%d/%d). Cannot register %s\n", 
               registry_count, MAX_BRIDGE_RESOURCES, signalK_path);
        pthread_mutex_unlock(&reg_mutex);
        return -1;
    }

    // Warning when approaching limit
    if (registry_count >= (MAX_BRIDGE_RESOURCES * 0.8)) {
        printf("[Bridge] Warning: Registry nearly full (%d/%d). Consider increasing MAX_BRIDGE_RESOURCES\n", 
               registry_count, MAX_BRIDGE_RESOURCES);
    }

    bridge_resource_t *res = &registry[registry_count++];
    res->objectId = objId;
    res->instanceId = instId;
    res->resourceId = resId;
    strncpy(res->signalK_path, signalK_path, sizeof(res->signalK_path) - 1);
    res->value[0] = '\0';
    res->observed = 0;

    printf("[Bridge] Registered mapping %d: %s -> Object %d/%d/%d\n", 
           registry_count, signalK_path, objId, instId, resId);

    pthread_mutex_unlock(&reg_mutex);
    return 0;
}

void bridge_update(const char *signalK_path, const char *new_value) {
    pthread_mutex_lock(&reg_mutex);

    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].signalK_path, signalK_path) == 0) {
            strncpy(registry[i].value, new_value, sizeof(registry[i].value) - 1);

            if (registry[i].observed) {
                lwm2m_uri_t uri;
                lwm2m_stringToUri(NULL, 0, &uri); // reset
                uri.objectId = registry[i].objectId;
                uri.instanceId = registry[i].instanceId;
                uri.resourceId = registry[i].resourceId;

                lwm2m_resource_value_changed(NULL, &uri); // will notify server
            }
            break;
        }
    }

    pthread_mutex_unlock(&reg_mutex);
}

const char *bridge_read(uint16_t objId, uint16_t instId, uint16_t resId) {
    const char *result = NULL;

    pthread_mutex_lock(&reg_mutex);

    for (int i = 0; i < registry_count; i++) {
        if (registry[i].objectId == objId &&
            registry[i].instanceId == instId &&
            registry[i].resourceId == resId) {
            result = registry[i].value;
            break;
        }
    }

    pthread_mutex_unlock(&reg_mutex);
    return result;
}

int bridge_write(uint16_t objId, uint16_t instId, uint16_t resId, const char *value) {
    int rc = -1;

    pthread_mutex_lock(&reg_mutex);

    for (int i = 0; i < registry_count; i++) {
        if (registry[i].objectId == objId &&
            registry[i].instanceId == instId &&
            registry[i].resourceId == resId) {
            strncpy(registry[i].value, value, sizeof(registry[i].value) - 1);
            rc = 0;
            // TODO: Optionally forward this write to Signal K over websocket
            break;
        }
    }

    pthread_mutex_unlock(&reg_mutex);
    return rc;
}
