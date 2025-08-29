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

    for (i = 0 ; i < *numDataP ; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
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

    for (i = 0 ; i < numData ; i++)
    {
        switch (dataArray[i].id)
        {
        case 5822: // Measurement Period
            if (lwm2m_data_decode_int(dataArray + i, (int64_t*)&targetP->measurementPeriod) == 1)
            {
                targetP->timestamp = time(NULL);
                return COAP_204_CHANGED;
            }
            else
            {
                return COAP_400_BAD_REQUEST;
            }
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

    switch (resourceId)
    {
    case 5810: // Reset Cumulative Energy
        targetP->cumulativeEnergy = 0.0;
        targetP->lastReset = time(NULL);
        targetP->timestamp = targetP->lastReset;
        printf("[Energy] Reset cumulative energy for instance %d (%s)\n", 
               instanceId, targetP->applicationType);
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
    lwm2m_object_t * energyObj;

    energyObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != energyObj)
    {
        energy_instance_t * targetP;

        memset(energyObj, 0, sizeof(lwm2m_object_t));

        energyObj->objID = 3331;
        energyObj->readFunc = prv_read;
        energyObj->writeFunc = prv_write;
        energyObj->createFunc = prv_create;
        energyObj->deleteFunc = prv_delete;
        energyObj->executeFunc = prv_execute;

        // Create initial instances for marine energy monitoring

        // Instance 0: Solar Generation Energy
        targetP = (energy_instance_t *)lwm2m_malloc(sizeof(energy_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(energy_instance_t));
        targetP->shortID = 0;
        targetP->cumulativeEnergy = 1250.0; // 1.25 kWh daily
        targetP->currentPower = 85.0;        // 85W current
        targetP->powerFactor = 1.0;          // DC system
        targetP->measurementPeriod = 300;    // 5 minutes
        strcpy(targetP->applicationType, "Solar Generation");
        targetP->timestamp = time(NULL);
        targetP->lastReset = targetP->timestamp - (24 * 3600); // Reset daily
        energyObj->instanceList = LWM2M_LIST_ADD(energyObj->instanceList, targetP);

        // Instance 1: Shore Power Consumption
        targetP = (energy_instance_t *)lwm2m_malloc(sizeof(energy_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(energy_instance_t));
        targetP->shortID = 1;
        targetP->cumulativeEnergy = 0.0;     // Not connected
        targetP->currentPower = 0.0;
        targetP->powerFactor = 0.95;         // AC system
        targetP->measurementPeriod = 60;     // 1 minute
        strcpy(targetP->applicationType, "Shore Power Consumption");
        targetP->timestamp = time(NULL);
        targetP->lastReset = targetP->timestamp;
        energyObj->instanceList = LWM2M_LIST_ADD(energyObj->instanceList, targetP);

        // Instance 2: House Load Consumption
        targetP = (energy_instance_t *)lwm2m_malloc(sizeof(energy_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(energy_instance_t));
        targetP->shortID = 2;
        targetP->cumulativeEnergy = 450.0;   // Daily house consumption
        targetP->currentPower = 65.0;        // Current house load
        targetP->powerFactor = 0.9;          // Mixed AC/DC loads
        targetP->measurementPeriod = 60;     // 1 minute
        strcpy(targetP->applicationType, "House Load Consumption");
        targetP->timestamp = time(NULL);
        targetP->lastReset = targetP->timestamp - (24 * 3600);
        energyObj->instanceList = LWM2M_LIST_ADD(energyObj->instanceList, targetP);

        // Instance 3: Engine Charging Energy
        targetP = (energy_instance_t *)lwm2m_malloc(sizeof(energy_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(energy_instance_t));
        targetP->shortID = 3;
        targetP->cumulativeEnergy = 0.0;     // Engine not running
        targetP->currentPower = 0.0;
        targetP->powerFactor = 1.0;          // DC charging
        targetP->measurementPeriod = 60;
        strcpy(targetP->applicationType, "Engine Alternator Charging");
        targetP->timestamp = time(NULL);
        targetP->lastReset = targetP->timestamp;
        energyObj->instanceList = LWM2M_LIST_ADD(energyObj->instanceList, targetP);

        printf("[Energy] Initialized with marine energy tracking instances\n");

        // Register SignalK bridge mappings for energy tracking
        bridge_register(3331, 0, 5805, "electrical.solar.cumulativeEnergy");
        bridge_register(3331, 0, 5800, "electrical.solar.panelsPower");
        bridge_register(3331, 2, 5805, "electrical.loads.total.cumulativeEnergy");
        bridge_register(3331, 2, 5800, "electrical.loads.total.power");
    }

    return energyObj;

error:
    lwm2m_free(energyObj);
    return NULL;
}

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
