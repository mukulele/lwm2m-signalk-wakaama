/*******************************************************************************
 * Bridge Object Mock Implementation for Unit Testing
 * 
 * Simplified version of bridge_object.c that removes LwM2M dependencies
 * for isolated unit testing of bridge logic
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_BRIDGE_RESOURCES 128
#define MAX_SIGNALK_PATH_LEN 256

typedef struct {
    uint16_t objId;
    uint16_t instId;
    uint16_t resId;
    char signalK_path[MAX_SIGNALK_PATH_LEN];
    char last_value[64];
    int is_registered;
} bridge_resource_t;

static bridge_resource_t registry[MAX_BRIDGE_RESOURCES];
static int registry_count = 0;
static pthread_mutex_t reg_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Mock URI structure for testing */
typedef struct {
    uint16_t objectId;
    uint16_t instanceId;
    uint16_t resourceId;
} lwm2m_uri_t;

/* Mock functions to replace LwM2M dependencies */
static int lwm2m_stringToUri(const char* uriString, size_t uriLength, lwm2m_uri_t* uriP) {
    (void)uriString;
    (void)uriLength;
    if (uriP) {
        uriP->objectId = 3300;
        uriP->instanceId = 0;
        uriP->resourceId = 5700;
    }
    return 1; /* Success */
}

static void lwm2m_resource_value_changed(void* context, lwm2m_uri_t* uriP) {
    (void)context;
    (void)uriP;
    /* Mock notification - in real implementation this notifies LwM2M server */
    printf("[MOCK] Resource value changed notification sent\n");
}

/* Bridge Object API Implementation */

void bridge_init(void) {
    pthread_mutex_lock(&reg_mutex);
    registry_count = 0;
    memset(registry, 0, sizeof(registry));
    pthread_mutex_unlock(&reg_mutex);
    printf("[Bridge] Initialized bridge registry\n");
}

int bridge_register(uint16_t objId, uint16_t instId, uint16_t resId, const char *signalK_path) {
    if (!signalK_path || strlen(signalK_path) == 0) {
        printf("[Bridge] Error: Invalid SignalK path\n");
        return -1;
    }

    if (strlen(signalK_path) >= MAX_SIGNALK_PATH_LEN) {
        printf("[Bridge] Error: SignalK path too long (%zu chars, max %d)\n", 
               strlen(signalK_path), MAX_SIGNALK_PATH_LEN - 1);
        return -1;
    }

    pthread_mutex_lock(&reg_mutex);

    if (registry_count >= MAX_BRIDGE_RESOURCES) {
        printf("[Bridge] Warning: Registry full (%d/%d). Cannot register %s\n", 
               registry_count, MAX_BRIDGE_RESOURCES, signalK_path);
        pthread_mutex_unlock(&reg_mutex);
        return -1;
    }

    /* Check for duplicate registration */
    for (int i = 0; i < registry_count; i++) {
        if (registry[i].objId == objId && 
            registry[i].instId == instId && 
            registry[i].resId == resId) {
            printf("[Bridge] Warning: Resource %d/%d/%d already registered\n", 
                   objId, instId, resId);
            /* Update the path instead of failing */
            strncpy(registry[i].signalK_path, signalK_path, MAX_SIGNALK_PATH_LEN - 1);
            registry[i].signalK_path[MAX_SIGNALK_PATH_LEN - 1] = '\0';
            pthread_mutex_unlock(&reg_mutex);
            return 0;
        }
    }

    /* Warning when approaching limit */
    if (registry_count >= (MAX_BRIDGE_RESOURCES * 0.8)) {
        printf("[Bridge] Warning: Registry nearly full (%d/%d). Consider increasing MAX_BRIDGE_RESOURCES\n", 
               registry_count, MAX_BRIDGE_RESOURCES);
    }

    /* Add new registration */
    bridge_resource_t *entry = &registry[registry_count];
    entry->objId = objId;
    entry->instId = instId;
    entry->resId = resId;
    strncpy(entry->signalK_path, signalK_path, MAX_SIGNALK_PATH_LEN - 1);
    entry->signalK_path[MAX_SIGNALK_PATH_LEN - 1] = '\0';
    entry->is_registered = 1;
    entry->last_value[0] = '\0';

    registry_count++;

    printf("[Bridge] Registered: %d/%d/%d -> %s (%d/%d)\n", 
           objId, instId, resId, signalK_path, registry_count, MAX_BRIDGE_RESOURCES);

    pthread_mutex_unlock(&reg_mutex);
    return 0;
}

void bridge_update(const char *signalK_path, const char *value) {
    if (!signalK_path || !value) {
        printf("[Bridge] Error: NULL parameters in bridge_update\n");
        return;
    }

    if (strlen(signalK_path) == 0) {
        printf("[Bridge] Error: Empty SignalK path\n");
        return;
    }

    pthread_mutex_lock(&reg_mutex);

    /* Find the registered resource */
    bridge_resource_t *found = NULL;
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].signalK_path, signalK_path) == 0) {
            found = &registry[i];
            break;
        }
    }

    if (!found) {
        printf("[Bridge] Warning: SignalK path '%s' not registered\n", signalK_path);
        pthread_mutex_unlock(&reg_mutex);
        return;
    }

    /* Update the value */
    strncpy(found->last_value, value, sizeof(found->last_value) - 1);
    found->last_value[sizeof(found->last_value) - 1] = '\0';

    printf("[Bridge] Updated: %s = %s (Object %d/%d/%d)\n", 
           signalK_path, value, found->objId, found->instId, found->resId);

    /* Mock LwM2M notification */
    lwm2m_uri_t uri;
    char uri_string[64];
    snprintf(uri_string, sizeof(uri_string), "/%d/%d/%d", 
             found->objId, found->instId, found->resId);
    
    if (lwm2m_stringToUri(uri_string, strlen(uri_string), &uri)) {
        lwm2m_resource_value_changed(NULL, &uri);
    }

    pthread_mutex_unlock(&reg_mutex);
}

/* Additional utility functions for testing */

int bridge_get_registry_count(void) {
    pthread_mutex_lock(&reg_mutex);
    int count = registry_count;
    pthread_mutex_unlock(&reg_mutex);
    return count;
}

int bridge_find_resource(const char *signalK_path, uint16_t *objId, uint16_t *instId, uint16_t *resId) {
    if (!signalK_path || !objId || !instId || !resId) {
        return -1;
    }

    pthread_mutex_lock(&reg_mutex);

    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].signalK_path, signalK_path) == 0) {
            *objId = registry[i].objId;
            *instId = registry[i].instId;
            *resId = registry[i].resId;
            pthread_mutex_unlock(&reg_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&reg_mutex);
    return -1;
}

const char* bridge_get_last_value(const char *signalK_path) {
    if (!signalK_path) {
        return NULL;
    }

    pthread_mutex_lock(&reg_mutex);

    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].signalK_path, signalK_path) == 0) {
            pthread_mutex_unlock(&reg_mutex);
            return registry[i].last_value;
        }
    }

    pthread_mutex_unlock(&reg_mutex);
    return NULL;
}

void bridge_print_registry(void) {
    pthread_mutex_lock(&reg_mutex);
    
    printf("[Bridge] Registry contents (%d/%d):\n", registry_count, MAX_BRIDGE_RESOURCES);
    for (int i = 0; i < registry_count; i++) {
        printf("  [%d] %d/%d/%d -> %s = '%s'\n", 
               i, registry[i].objId, registry[i].instId, registry[i].resId,
               registry[i].signalK_path, registry[i].last_value);
    }
    
    pthread_mutex_unlock(&reg_mutex);
}
