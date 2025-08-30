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
 * Name               | ID | Oper.| Instances | Mand. |  Type   | Range | Units | Description |
 *--------------------+----+------+-----------+-------+---------+-------+-------+-------------|
 * Sensor Value       |5700|  R   |  Single   |  Yes  | String  |       |       | Sensor data |
 * Sensor Units       |5701|  R   |  Single   |  No   | String  |       |       | Units       |
 */

#include "liblwm2m.h"
#include "bridge_object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
    char path[128];     // SignalK path mapped here
    char value[64];     // Last known value (string)
    char units[16];     // Units (optional)
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
    sensor_instance_t *inst = (sensor_instance_t *)objectP->userData;
    if (!inst) return COAP_404_NOT_FOUND;

    /* unused parameter */
    (void)contextP;

    if (*numDataP == 0)
    {
        // Server requested all resources
        *dataArray = lwm2m_data_new(2);
        if (*dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 2;
        (*dataArray)[0].id = 5700;  // Sensor Value
        (*dataArray)[1].id = 5701;  // Units
    }

    for (int i = 0; i < *numDataP; i++)
    {
        switch ((*dataArray)[i].id)
        {
        case 5700: // Sensor Value
            lwm2m_data_encode_string(inst->value, &(*dataArray)[i]);
            break;
        case 5701: // Units
            lwm2m_data_encode_string(inst->units, &(*dataArray)[i]);
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
    sensor_instance_t *inst = (sensor_instance_t *)objectP->userData;
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
    
    if (objectP->userData)
    {
        free(objectP->userData);
        objectP->userData = NULL;
    }
    return COAP_202_DELETED;
}

// -----------------------------------------------------------------------------
// Public constructor
// -----------------------------------------------------------------------------
lwm2m_object_t * get_object_generic_sensor(const char *path, const char *units)
{
    lwm2m_object_t *obj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (!obj) return NULL;
    memset(obj, 0, sizeof(lwm2m_object_t));

    obj->objID = 3300;

    sensor_instance_t *inst = (sensor_instance_t *)malloc(sizeof(sensor_instance_t));
    if (!inst)
    {
        lwm2m_free(obj);
        return NULL;
    }
    memset(inst, 0, sizeof(sensor_instance_t));

    strncpy(inst->path, path, sizeof(inst->path) - 1);
    strncpy(inst->units, units ? units : "", sizeof(inst->units) - 1);
    strcpy(inst->value, "0"); // default

    // Add single instance 0
    obj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
    if (obj->instanceList != NULL) {
        memset(obj->instanceList, 0, sizeof(lwm2m_list_t));
        obj->instanceList->id = 0;
    }

    obj->readFunc     = prv_read;
    obj->writeFunc    = prv_write;
    obj->discoverFunc = prv_discover;
    obj->deleteFunc   = prv_delete;

    obj->userData = inst;

    // Register with bridge (maps SignalK path → LwM2M resource)
    // Note: Bridge registration will be done after all objects are created
    // bridge_register(3300, 0, 5700, path);

    return obj;
}

void free_object_generic_sensor(lwm2m_object_t *objectP)
{
    if (objectP != NULL)
    {
        if (objectP->userData != NULL)
        {
            free(objectP->userData);
            objectP->userData = NULL;
        }
        lwm2m_list_free(objectP->instanceList);
        lwm2m_free(objectP);
    }
}
