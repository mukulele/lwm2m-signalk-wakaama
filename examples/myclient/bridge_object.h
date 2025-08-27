#ifndef BRIDGE_OBJECT_H
#define BRIDGE_OBJECT_H

#include "liblwm2m.h"
#include <stdint.h>

/**
 * Bridge Resource Mapping
 * -----------------------
 * This structure represents a single LwM2M resource in the device,
 * which is mapped to a Signal K path (JSON key).
 *
 * Example:
 *   Signal K: "environment.wind.speedApparent"
 *   LwM2M:    Object 3300 (Generic Sensor), Instance 0, Resource 5700
 */
typedef struct {
    uint16_t objectId;
    uint16_t instanceId;
    uint16_t resourceId;
    char     signalK_path[128];   // JSON path from Signal K server
    char     value[64];           // cached string value
    int      observed;            // 1 = resource has an active OBSERVE
} bridge_resource_t;


/**
 * Initialize the bridge registry.
 * Call this once at startup before using other bridge functions.
 */
void bridge_init(void);


/**
 * Register a new mapping between Signal K path and LwM2M resource.
 * Returns 0 on success, -1 if the registry is full.
 */
int bridge_register(uint16_t objId,
                    uint16_t instId,
                    uint16_t resId,
                    const char *signalK_path);


/**
 * Update resource value (from Signal K JSON).
 * If the resource is observed, Wakaama notify is triggered.
 */
void bridge_update(const char *signalK_path, const char *new_value);


/**
 * Read current value of a mapped resource.
 * Returns pointer to internal string buffer, or NULL if not found.
 */
const char *bridge_read(uint16_t objId, uint16_t instId, uint16_t resId);


/**
 * Write a value into a mapped resource (from LwM2M server).
 * This can be used to send commands back to Signal K if desired.
 */
int bridge_write(uint16_t objId, uint16_t instId, uint16_t resId, const char *value);

#endif /* BRIDGE_OBJECT_H */
