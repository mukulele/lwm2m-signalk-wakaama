/*******************************************************************************
 *
 * Copyright (c) 2025 Marine IoT Systems
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * Contributors:
 *    Marine IoT Systems - SignalK LwM2M Generic Sensor implementation
 *
 *******************************************************************************/

/*
 * Object     |      | Multiple  |     | Description                   |
 * Name       |  ID  | Instances |Mand.|                               |
 *------------+------+-----------+-----+-------------------------------+
 * Generic    | 3300 |    Yes    |  No | Generic sensor for marine     |
 * Sensor     |      |           |     | environmental monitoring      |
 *
 * Resources:
 * Name               | ID   | Oper.| Instances | Mand. |  Type   | Range | Units | Description         |
 *--------------------+------+------|-----------+-------+---------+-------+-------+---------------------|
 * Sensor Value       | 5700 |  R   |  Single   |  Yes  | String  |       |       | Sensor data         |
 * Sensor Units       | 5701 |  R   |  Single   |  No   | String  |       |       | Units               |
 * Application Type   | 5750 |  R   |  Single   |  No   | String  |       |       | Instance name/label |
 */

#include "liblwm2m.h"
#include "bridge_object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct sensor_instance {
    struct sensor_instance *next;
    uint16_t id;
    char path[128];     // SignalK path mapped here
    char value[64];     // Last known value (string)
    char units[16];     // Units (optional)
    char appType[32];   // Application Type (instance name/label)
} sensor_instance_t;

// -----------------------------------------------------------------------------
// READ callback
// -----------------------------------------------------------------------------
static uint8_t prv_read(lwm2m_context_t *contextP,
                        uint16_t instanceId,
                        int *numDataP,
                        lwm2m_data_t **dataArray,
                        lwm2m_object_t *objectP)
{
    sensor_instance_t *inst = NULL;
    for (sensor_instance_t *cur = (sensor_instance_t *)objectP->instanceList; cur != NULL; cur = cur->next) {
        if (cur->id == instanceId) {
            inst = cur;
            break;
        }
    }
    if (!inst) return COAP_404_NOT_FOUND;

    /* unused parameter */
    (void)contextP;

    if (*numDataP == 0)
    {
        // Server requested all resources
        *dataArray = lwm2m_data_new(3);
        if (*dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 3;
        (*dataArray)[0].id = 5700;  // Sensor Value
        (*dataArray)[1].id = 5701;  // Units
        (*dataArray)[2].id = 5750;  // Application Type
    }

    for (int i = 0; i < *numDataP; i++)
    {
        switch ((*dataArray)[i].id)
        {
        case 5700: // Sensor Value
        {
            char *endptr = NULL;
            long int_val = strtol(inst->value, &endptr, 10);
            if (endptr && *endptr == '\0') {
                lwm2m_data_encode_int(int_val, &(*dataArray)[i]);
            } else {
                endptr = NULL;
                double float_val = strtod(inst->value, &endptr);
                if (endptr && *endptr == '\0') {
                    lwm2m_data_encode_float(float_val, &(*dataArray)[i]);
                } else {
                    lwm2m_data_encode_string(inst->value, &(*dataArray)[i]);
                }
            }
            break;
        }
        case 5701: // Units
            lwm2m_data_encode_string(inst->units, &(*dataArray)[i]);
            break;
        case 5750: // Application Type
            lwm2m_data_encode_string(inst->appType, &(*dataArray)[i]);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

// -----------------------------------------------------------------------------
// WRITE callback
// -----------------------------------------------------------------------------
static uint8_t prv_write(lwm2m_context_t *contextP,
                         uint16_t instanceId,
                         int numData,
                         lwm2m_data_t *dataArray,
                         lwm2m_object_t *objectP,
                         lwm2m_write_type_t writeType)
{
    sensor_instance_t *inst = NULL;
    for (sensor_instance_t *cur = (sensor_instance_t *)objectP->instanceList; cur != NULL; cur = cur->next) {
        if (cur->id == instanceId) {
            inst = cur;
            break;
        }
    }
    if (!inst) return COAP_404_NOT_FOUND;

    /* unused parameters */
    (void)contextP;
    (void)writeType;

    for (int i = 0; i < numData; i++)
    {
        switch (dataArray[i].id)
        {
        case 5700: // Sensor Value
        {
            if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                char *val = (char *)malloc(dataArray[i].value.asBuffer.length + 1);
                if (val) {
                    memcpy(val, dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                    val[dataArray[i].value.asBuffer.length] = '\0';
                    strncpy(inst->value, val, sizeof(inst->value) - 1);
                    inst->value[sizeof(inst->value) - 1] = '\0';
                    free(val);
                    // Push back into bridge
                    bridge_update(inst->path, inst->value);
                    // Notify observers
                    lwm2m_uri_t uri;
                    lwm2m_stringToUri(NULL, 0, &uri);
                    uri.objectId = 3300;
                    uri.instanceId = inst->id;
                    uri.resourceId = 5700;
                    lwm2m_resource_value_changed(contextP, &uri);
                }
            }
            break;
        }
        case 5701: // Units
        {
            if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                char *val = (char *)malloc(dataArray[i].value.asBuffer.length + 1);
                if (val) {
                    memcpy(val, dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                    val[dataArray[i].value.asBuffer.length] = '\0';
                    strncpy(inst->units, val, sizeof(inst->units) - 1);
                    inst->units[sizeof(inst->units) - 1] = '\0';
                    free(val);
                }
            }
            break;
        }
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_204_CHANGED;
}

// -----------------------------------------------------------------------------
// DISCOVER callback
// -----------------------------------------------------------------------------
static uint8_t prv_discover(lwm2m_context_t *contextP,
                            uint16_t instanceId,
                            int *numDataP,
                            lwm2m_data_t **dataArray,
                            lwm2m_object_t *objectP)
{
    /* unused parameter */
    (void)contextP;
    
    // Just advertise resources 5700 and 5701
    if (*numDataP == 0)
    {
        *dataArray = lwm2m_data_new(2);
        if (*dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 2;
        (*dataArray)[0].id = 5700;
        (*dataArray)[1].id = 5701;
    }
    return COAP_205_CONTENT;
}

// -----------------------------------------------------------------------------
// DELETE callback (server deletes instance)
// -----------------------------------------------------------------------------
static uint8_t prv_delete(lwm2m_context_t *contextP,
                          uint16_t id,
                          lwm2m_object_t *objectP)
{
    /* unused parameter */
    (void)contextP;
    
    sensor_instance_t **prev = (sensor_instance_t **)&objectP->instanceList;
    sensor_instance_t *cur = (sensor_instance_t *)objectP->instanceList;
    while (cur) {
        if (cur->id == id) {
            *prev = cur->next;
            free(cur);
            return COAP_202_DELETED;
        }
        prev = &cur->next;
        cur = cur->next;
    }
    return COAP_404_NOT_FOUND;
}

// -----------------------------------------------------------------------------
// Public constructor
// -----------------------------------------------------------------------------
// Create a generic sensor object with no instances yet
lwm2m_object_t * get_object_generic_sensor(void)
{
    lwm2m_object_t *obj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (!obj) return NULL;
    memset(obj, 0, sizeof(lwm2m_object_t));

    obj->objID = 3300;
    obj->instanceList = NULL; // No instances at startup
    obj->readFunc     = prv_read;
    obj->writeFunc    = prv_write;
    obj->discoverFunc = prv_discover;
    obj->deleteFunc   = prv_delete;
    printf("[Generic Sensor] Created with no instances. Instances will be added dynamically.\n");
    return obj;
}

// Add a new instance for a SignalK path
sensor_instance_t * add_generic_sensor_instance(lwm2m_object_t *obj, uint16_t instanceId, const char *path, const char *units)
{
    sensor_instance_t *inst = (sensor_instance_t *)malloc(sizeof(sensor_instance_t));
    if (!inst) return NULL;
    memset(inst, 0, sizeof(sensor_instance_t));
    inst->id = instanceId;
    strncpy(inst->path, path, sizeof(inst->path) - 1);
    strncpy(inst->units, units ? units : "", sizeof(inst->units) - 1);
    strcpy(inst->value, "0"); // default
    strncpy(inst->appType, path, sizeof(inst->appType) - 1); // Default: use path as label
    inst->next = (sensor_instance_t *)obj->instanceList;
    obj->instanceList = (lwm2m_list_t *)inst;
    // Only register mapping if not already present
    bridge_register(3300, instanceId, 5700, path);
    printf("[Generic Sensor] Dynamically created instance %d for path %s\n", instanceId, path);
    return inst;
}

void free_object_generic_sensor(lwm2m_object_t *objectP)
{
    if (objectP != NULL)
    {
        sensor_instance_t *cur = (sensor_instance_t *)objectP->instanceList;
        while (cur) {
            sensor_instance_t *next = cur->next;
            free(cur);
            cur = next;
        }
        objectP->instanceList = NULL;
        lwm2m_free(objectP);
    }
}
