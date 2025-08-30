/*******************************************************************************
 *
 * Copyright (c) 2025 Marine IoT Systems
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * Contributors:
 *    Marine IoT Systems - SignalK LwM2M Power Measurement implementation
 *
 *******************************************************************************/

/*
 * Object     |      | Multiple  |     | Description                   |
 * Name       |  ID  | Instances |Mand.|                               |
 *------------+------+-----------+-----+-------------------------------+
 * Power      | 3305 |    Yes    |  No | Power measurement for marine  |
 * Measurement|      |           |     | electrical systems            |
 *
 * Resources:
 * Name                       | ID | Oper.| Instances | Mand. |  Type   | Range | Units | Description |
 * ---------------------------+----+------+-----------+-------+---------+-------+-------+-------------|
 * Instantaneous Active Power|5800|  R   |  Single   |  Yes  | Float   |       |   W   | Current active power |
 * Min Measured Active Power |5801|  R   |  Single   |  No   | Float   |       |   W   | Min active power |
 * Max Measured Active Power |5802|  R   |  Single   |  No   | Float   |       |   W   | Max active power |
 * Cumulative Active Power   |5805|  R   |  Single   |  No   | Float   |       |  Wh   | Total energy consumed |
 * Instantaneous Reactive Pwr|5810|  R   |  Single   |  No   | Float   |       |  var  | Current reactive power |
 * Power Factor              |5820|  R   |  Single   |  No   | Float   |       |       | Power factor |
 * Reset Min/Max Values      |5605|  E   |  Single   |  No   | Opaque  |       |       | Reset min/max |
 * Reset Cumulative Energy   |5822|  E   |  Single   |  No   | Opaque  |       |       | Reset cumulative |
 * Application Type          |5750|  RW  |  Single   |  No   | String  |       |       | Description |
 * Timestamp                 |5518|  R   |  Single   |  No   | Time    |       |   s   | Measurement time |
 */

#include "liblwm2m.h"
#include "bridge_object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwM2M_calloc_object() function will automatically free the userdata when the object is deleted.
 */
typedef struct _power_measurement_instance_
{
    struct _power_measurement_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;                             // matches lwm2m_list_t::id
    double   sensorValue;
    char     units[32];
    double   minMeasuredValue;
    double   maxMeasuredValue;
    double   minRangeValue;
    double   maxRangeValue;
    char     applicationType[64];
    time_t   timestamp;
} power_measurement_instance_t;

// Forward declarations
static uint8_t prv_delete(lwm2m_context_t * contextP, uint16_t instanceId, lwm2m_object_t * objectP);
static uint8_t prv_write(lwm2m_context_t * contextP, uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP, lwm2m_write_type_t writeType);

static uint8_t prv_read(lwm2m_context_t * contextP,
                       uint16_t instanceId,
                       int * numDataP,
                       lwm2m_data_t ** dataArrayP,
                       lwm2m_object_t * objectP)
{
    power_measurement_instance_t * targetP;
    int i;

    targetP = (power_measurement_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(8);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 8;
        (*dataArrayP)[0].id = 5700; // Sensor Value
        (*dataArrayP)[1].id = 5701; // Sensor Units
        (*dataArrayP)[2].id = 5602; // Min Measured Value
        (*dataArrayP)[3].id = 5603; // Max Measured Value
        (*dataArrayP)[4].id = 5604; // Min Range Value
        (*dataArrayP)[5].id = 5605; // Max Range Value
        (*dataArrayP)[6].id = 5750; // Application Type
        (*dataArrayP)[7].id = 5518; // Timestamp
    }

    for (i = 0 ; i < *numDataP ; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
        case 5700: // Sensor Value
            lwm2m_data_encode_float(targetP->sensorValue, *dataArrayP + i);
            break;
        case 5701: // Sensor Units
            lwm2m_data_encode_string(targetP->units, *dataArrayP + i);
            break;
        case 5602: // Min Measured Value
            lwm2m_data_encode_float(targetP->minMeasuredValue, *dataArrayP + i);
            break;
        case 5603: // Max Measured Value
            lwm2m_data_encode_float(targetP->maxMeasuredValue, *dataArrayP + i);
            break;
        case 5604: // Min Range Value
            lwm2m_data_encode_float(targetP->minRangeValue, *dataArrayP + i);
            break;
        case 5605: // Max Range Value
            lwm2m_data_encode_float(targetP->maxRangeValue, *dataArrayP + i);
            break;
        case 5750: // Application Type
            lwm2m_data_encode_string(targetP->applicationType, *dataArrayP + i);
            break;
        case 5518: // Timestamp
            lwm2m_data_encode_int(targetP->timestamp, *dataArrayP + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_execute(lwm2m_context_t * contextP,
                          uint16_t instanceId,
                          uint16_t resourceId,
                          uint8_t * buffer,
                          int length,
                          lwm2m_object_t * objectP)
{
    power_measurement_instance_t * targetP;

    targetP = (power_measurement_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case 5605: // Reset Min/Max
        targetP->minMeasuredValue = targetP->sensorValue;
        targetP->maxMeasuredValue = targetP->sensorValue;
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_write(lwm2m_context_t * contextP,
                        uint16_t instanceId,
                        int numData,
                        lwm2m_data_t * dataArray,
                        lwm2m_object_t * objectP,
                        lwm2m_write_type_t writeType)
{
    power_measurement_instance_t * targetP;
    int i;

    targetP = (power_measurement_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (writeType == LWM2M_WRITE_REPLACE_INSTANCE)
    {
        return COAP_501_NOT_IMPLEMENTED;
    }

    for (i = 0 ; i < numData ; i++)
    {
        switch (dataArray[i].id)
        {
        case 5700: // Sensor Value (writable for testing)
            if (lwm2m_data_decode_float(dataArray + i, &targetP->sensorValue) == 1)
            {
                // Update min/max
                if (targetP->sensorValue < targetP->minMeasuredValue)
                    targetP->minMeasuredValue = targetP->sensorValue;
                if (targetP->sensorValue > targetP->maxMeasuredValue)
                    targetP->maxMeasuredValue = targetP->sensorValue;
                
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

static uint8_t prv_create(lwm2m_context_t * contextP,
                         uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    power_measurement_instance_t * targetP;
    uint8_t result;

    targetP = (power_measurement_instance_t *)lwm2m_malloc(sizeof(power_measurement_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(power_measurement_instance_t));

    targetP->shortID = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(contextP, instanceId, numData, dataArray, objectP, LWM2M_WRITE_REPLACE_INSTANCE);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_delete(contextP, instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static uint8_t prv_delete(lwm2m_context_t * contextP,
                         uint16_t instanceId,
                         lwm2m_object_t * objectP)
{
    power_measurement_instance_t * targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, instanceId, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

// Helper function to update power measurement values from SignalK
void power_measurement_update_value(lwm2m_object_t * objectP, uint16_t instanceId, 
                                   double value, const char* units, const char* appType)
{
    power_measurement_instance_t * targetP;

    targetP = (power_measurement_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return;

    targetP->sensorValue = value;
    if (units) strncpy(targetP->units, units, sizeof(targetP->units) - 1);
    if (appType) strncpy(targetP->applicationType, appType, sizeof(targetP->applicationType) - 1);

    // Update min/max values
    if (value < targetP->minMeasuredValue || targetP->minMeasuredValue == 0)
        targetP->minMeasuredValue = value;
    if (value > targetP->maxMeasuredValue)
        targetP->maxMeasuredValue = value;

    targetP->timestamp = time(NULL);
}

lwm2m_object_t * get_power_measurement_object(void)
{
    lwm2m_object_t * powerObj;

    powerObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != powerObj)
    {
        int i;
        power_measurement_instance_t * targetP;

        memset(powerObj, 0, sizeof(lwm2m_object_t));

        powerObj->objID = 3305;
        powerObj->readFunc = prv_read;
        powerObj->writeFunc = prv_write;
        powerObj->createFunc = prv_create;
        powerObj->deleteFunc = prv_delete;
        powerObj->executeFunc = prv_execute;

        // Create initial instances for marine electrical system
        
        // Instance 0: House Battery Voltage
        targetP = (power_measurement_instance_t *)lwm2m_malloc(sizeof(power_measurement_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(power_measurement_instance_t));
        targetP->shortID = 0;
        targetP->sensorValue = 12.6;
        strcpy(targetP->units, "V");
        strcpy(targetP->applicationType, "House Battery Voltage");
        targetP->minRangeValue = 10.5;  // Low voltage alarm
        targetP->maxRangeValue = 15.0;  // High voltage protection
        targetP->minMeasuredValue = 12.6;
        targetP->maxMeasuredValue = 12.6;
        targetP->timestamp = time(NULL);
        powerObj->instanceList = LWM2M_LIST_ADD(powerObj->instanceList, targetP);

        // Instance 1: Engine Battery Voltage  
        targetP = (power_measurement_instance_t *)lwm2m_malloc(sizeof(power_measurement_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(power_measurement_instance_t));
        targetP->shortID = 1;
        targetP->sensorValue = 12.8;
        strcpy(targetP->units, "V");
        strcpy(targetP->applicationType, "Engine Battery Voltage");
        targetP->minRangeValue = 10.5;
        targetP->maxRangeValue = 15.0;
        targetP->minMeasuredValue = 12.8;
        targetP->maxMeasuredValue = 12.8;
        targetP->timestamp = time(NULL);
        powerObj->instanceList = LWM2M_LIST_ADD(powerObj->instanceList, targetP);

        // Instance 2: House Battery Current
        targetP = (power_measurement_instance_t *)lwm2m_malloc(sizeof(power_measurement_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(power_measurement_instance_t));
        targetP->shortID = 2;
        targetP->sensorValue = -5.2;  // Negative = discharging
        strcpy(targetP->units, "A");
        strcpy(targetP->applicationType, "House Battery Current");
        targetP->minRangeValue = -200.0;  // Max discharge current
        targetP->maxRangeValue = 100.0;   // Max charging current
        targetP->minMeasuredValue = -5.2;
        targetP->maxMeasuredValue = -5.2;
        targetP->timestamp = time(NULL);
        powerObj->instanceList = LWM2M_LIST_ADD(powerObj->instanceList, targetP);

        // Instance 10: Solar Panel Power
        targetP = (power_measurement_instance_t *)lwm2m_malloc(sizeof(power_measurement_instance_t));
        if (NULL == targetP) goto error;
        memset(targetP, 0, sizeof(power_measurement_instance_t));
        targetP->shortID = 10;
        targetP->sensorValue = 85.0;
        strcpy(targetP->units, "W");
        strcpy(targetP->applicationType, "Solar Panel Power");
        targetP->minRangeValue = 0.0;
        targetP->maxRangeValue = 400.0;  // Max panel capacity
        targetP->minMeasuredValue = 85.0;
        targetP->maxMeasuredValue = 85.0;
        targetP->timestamp = time(NULL);
        powerObj->instanceList = LWM2M_LIST_ADD(powerObj->instanceList, targetP);

        printf("[Power Measurement] Initialized with marine electrical monitoring instances\n");

        // Register SignalK bridge mappings for electrical system
        bridge_register(3305, 0, 5700, "electrical.batteries.house.voltage");
        bridge_register(3305, 1, 5700, "electrical.batteries.engine.voltage");
        bridge_register(3305, 2, 5700, "electrical.batteries.house.current");
        bridge_register(3305, 10, 5700, "electrical.solar.panelsPower");
    }

    return powerObj;

error:
    lwm2m_free(powerObj);
    return NULL;
}

void free_power_measurement_object(lwm2m_object_t * objectP)
{
    LWM2M_LIST_FREE(objectP->instanceList);
    if (objectP->userData != NULL)
    {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    lwm2m_free(objectP);
}
