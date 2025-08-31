/*******************************************************************************
 *
 * Copyright (c) 2025 Marine IoT Systems
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * Contributors:
 *    Marine IoT Systems - SignalK LwM2M Energy object implementation
 *
 *******************************************************************************/

/*
 * Object     |      | Multiple  |     | Description                   |
 * Name       |  ID  | Instances |Mand.|                               |
 *------------+------+-----------+-----+-------------------------------+
 * Energy     | 3331 |    Yes    |  No | Energy measurement and        |
 *            |      |           |     | cumulative tracking           |
 *
 * Resources:
 * Name                    | ID | Oper.| Instances | Mand. |  Type   | Range | Units | Description |
 * ------------------------+----+------+-----------+-------+---------+-------+-------+-------------|
 * Cumulative Energy       |5805|  R   |  Single   |  Yes  | Float   |       |  Wh   | Cumulative energy |
 * Power                   |5800|  R   |  Single   |  No   | Float   |       |   W   | Current power |
 * Power Factor            |5820|  R   |  Single   |  No   | Float   |0.0-1.0|       | Power factor |
 * Measurement Period      |5822|  R/W |  Single   |  No   | Integer |       |   s   | Measurement period |
 * Reset Cumulative Energy |5810|  E   |  Single   |  No   | Opaque  |       |       | Reset energy counter |
 * Application Type        |5750|  R   |  Single   |  No   | String  |       |       | Application description |
 * Timestamp               |5518|  R   |  Single   |  No   | Time    |       |   s   | Measurement timestamp |
 */

#include "liblwm2m.h"
#include "bridge_object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct _energy_instance_
{
    struct _energy_instance_ * next;     // matches lwm2m_list_t::next
    uint16_t shortID;                    // matches lwm2m_list_t::id
    double   cumulativeEnergy;          // Wh
    double   currentPower;              // W
    double   powerFactor;               // 0.0-1.0
    int      measurementPeriod;         // seconds
    char     applicationType[64];
    time_t   timestamp;
    time_t   lastReset;
} energy_instance_t;

// Forward declarations
static uint8_t prv_delete(lwm2m_context_t * contextP, uint16_t instanceId, lwm2m_object_t * objectP);
static uint8_t prv_write(lwm2m_context_t * contextP, uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP, lwm2m_write_type_t writeType);

static uint8_t prv_read(lwm2m_context_t * contextP,
                       uint16_t instanceId,
                       int * numDataP,
                       lwm2m_data_t ** dataArrayP,
                       lwm2m_object_t * objectP)
{
    energy_instance_t * targetP;
    int i;

    targetP = (energy_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(7);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 7;
        (*dataArrayP)[0].id = 5805; // Cumulative Energy
        (*dataArrayP)[1].id = 5800; // Power
        (*dataArrayP)[2].id = 5820; // Power Factor
        (*dataArrayP)[3].id = 5822; // Measurement Period
        (*dataArrayP)[4].id = 5750; // Application Type
        (*dataArrayP)[5].id = 5518; // Timestamp
        (*dataArrayP)[6].id = 5810; // Reset (placeholder for discovery)
    }


    for (i = 0 ; i < *numDataP ; i++) {
        switch ((*dataArrayP)[i].id) {
        case 5805: // Cumulative Energy
            lwm2m_data_encode_float(targetP->cumulativeEnergy, *dataArrayP + i);
            break;
        case 5800: // Power
            lwm2m_data_encode_float(targetP->currentPower, *dataArrayP + i);
            break;
        case 5820: // Power Factor
            lwm2m_data_encode_float(targetP->powerFactor, *dataArrayP + i);
            break;
        case 5822: // Measurement Period
            lwm2m_data_encode_int(targetP->measurementPeriod, *dataArrayP + i);
            break;
        case 5750: // Application Type
            lwm2m_data_encode_string(targetP->applicationType, *dataArrayP + i);
            break;
        case 5518: // Timestamp
            lwm2m_data_encode_int(targetP->timestamp, *dataArrayP + i);
            break;
        case 5810: // Reset (not readable, just for discovery)
            return COAP_405_METHOD_NOT_ALLOWED;
        default:
            return COAP_404_NOT_FOUND;
        }
    }
    return COAP_205_CONTENT;
}
static uint8_t prv_write(lwm2m_context_t * contextP,
                        uint16_t instanceId,
                        int numData,
                        lwm2m_data_t * dataArray,
                        lwm2m_object_t * objectP,
                        lwm2m_write_type_t writeType)
{
    energy_instance_t * targetP;
    int i;
    targetP = (energy_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    for (i = 0 ; i < numData ; i++) {
        switch (dataArray[i].id) {
        case 5822: // Measurement Period
            if (lwm2m_data_decode_int(dataArray + i, (int64_t*)&targetP->measurementPeriod) == 1) {
                targetP->timestamp = time(NULL);
            } else {
                return COAP_400_BAD_REQUEST;
            }
            break;
        default:
            return COAP_405_METHOD_NOT_ALLOWED;
        }
    }
    return COAP_204_CHANGED;
}

static uint8_t prv_execute(lwm2m_context_t * contextP,
                          uint16_t instanceId,
                          uint16_t resourceId,
                          uint8_t * buffer,
                          int length,
                          lwm2m_object_t * objectP)
{
    energy_instance_t * targetP;
    targetP = (energy_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    switch (resourceId) {
    case 5810: // Reset Cumulative Energy
        targetP->cumulativeEnergy = 0.0;
        targetP->lastReset = time(NULL);
        targetP->timestamp = targetP->lastReset;
        printf("[Energy] Reset cumulative energy for instance %d (%s)\n", instanceId, targetP->applicationType);
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_create(lwm2m_context_t * contextP,
                         uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    energy_instance_t * targetP;
    targetP = (energy_instance_t *)lwm2m_malloc(sizeof(energy_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(energy_instance_t));
    targetP->shortID = instanceId;
    targetP->measurementPeriod = 60; // Default 1 minute
    targetP->powerFactor = 1.0;      // Default unity power factor
    targetP->timestamp = time(NULL);
    targetP->lastReset = targetP->timestamp;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);
    return COAP_201_CREATED;
}

static uint8_t prv_delete(lwm2m_context_t * contextP,
                         uint16_t instanceId,
                         lwm2m_object_t * objectP)
{
    energy_instance_t * targetP;
    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, instanceId, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    lwm2m_free(targetP);
    return COAP_202_DELETED;
}


// Helper function to update energy values from SignalK
void energy_update_value(lwm2m_object_t * objectP, uint16_t instanceId, 
                        double cumulativeWh, double currentW, const char* appType)
{
    energy_instance_t * targetP;

    targetP = (energy_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return;

    targetP->cumulativeEnergy = cumulativeWh;
    targetP->currentPower = currentW;
    if (appType) strncpy(targetP->applicationType, appType, sizeof(targetP->applicationType) - 1);
    targetP->timestamp = time(NULL);
}

lwm2m_object_t * get_energy_object(void)
{
    lwm2m_object_t * energyObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
    if (!energyObj) return NULL;
    memset(energyObj, 0, sizeof(lwm2m_object_t));
    energyObj->objID = 3331;
    energyObj->instanceList = NULL; // No instances at startup
    energyObj->readFunc     = prv_read;
    energyObj->writeFunc    = prv_write;
    energyObj->createFunc   = prv_create;
    energyObj->deleteFunc   = prv_delete;
    energyObj->executeFunc  = prv_execute;
    printf("[Energy] Created with no instances. Instances will be added dynamically.\n");
    return energyObj;
}
// Dynamically create and register a new energy instance
// No custom dynamic registration helper needed. Use standard LwM2M create/delete and value update.

void free_energy_object(lwm2m_object_t * objectP)
{
    LWM2M_LIST_FREE(objectP->instanceList);
    if (objectP->userData != NULL)
    {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    lwm2m_free(objectP);
}
