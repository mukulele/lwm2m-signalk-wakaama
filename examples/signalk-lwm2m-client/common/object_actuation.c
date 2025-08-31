/*******************************************************************************
 *
 * Copyright (c) 2025 Marine IoT Systems
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * Contributors:
 *    Marine IoT Systems - SignalK LwM2M Actuation implementation
 *
 *******************************************************************************/

/*
 * Object     |      | Multiple  |     | Description                   |
 * Name       |  ID  | Instances |Mand.|                               |
 *------------+------+-----------+-----+-------------------------------+
 * Actuation  | 3306 |    Yes    |  No | Control actuators & switches  |
 *            |      |           |     | for marine electrical systems |
 *
 * Resources:
 * Name               | ID | Oper.| Instances | Mand. |  Type   | Range | Units | Description |
 *--------------------+----+------+-----------+-------+---------+-------+-------+-------------|
 * On/Off             |5850|  R/W |  Single   |  Yes  | Boolean |       |       | Switch state|
 * Dimmer             |5851|  R/W |  Single   |  No   | Integer | 0-100 |   %   | Dimmer level|
 * Units              |5701|  R   |  Single   |  No   | String  |       |       | Unit type   |
 * Application Type   |5750|  R   |  Single   |  No   | String  |       |       | Description |
 */

#include "liblwm2m.h"
#include "bridge_object.h"
#include "../websocket_client/signalk_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwM2M_calloc_object() function will automatically free the userdata when the object is deleted.
 */
typedef struct _actuation_instance_
{
    struct _actuation_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;                     // matches lwm2m_list_t::id
    bool     onOff;                       // Switch state
    int      dimmer;                      // Dimmer value 0-100
    char     units[32];                   // Unit description
    char     applicationType[64];         // Application description
    time_t   timestamp;                   // Last update timestamp
} actuation_instance_t;

/**
 * Map actuation instance ID to SignalK path
 * Based on the documented switch control instances in README.md
 */
static const char* get_signalk_path(uint16_t instance_id) {
    switch (instance_id) {
        case 0: return "electrical/switches/navigation/lights";
        case 1: return "electrical/switches/anchor/light";
        case 2: return "electrical/switches/bilgePump/main";
        case 3: return "electrical/switches/freshWaterPump";
        case 4: return "electrical/switches/cabin/lights";
        case 5: return "electrical/switches/windlass";
        default: 
            printf("[Actuation] Warning: Unknown instance %d, using generic path\n", instance_id);
            return "electrical/switches/unknown";
    }
}

// Forward declarations
static uint8_t prv_delete(lwm2m_context_t * contextP, uint16_t instanceId, lwm2m_object_t * objectP);
static uint8_t prv_write(lwm2m_context_t * contextP, uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP, lwm2m_write_type_t writeType);

static uint8_t prv_read(lwm2m_context_t * contextP,
                       uint16_t instanceId,
                       int * numDataP,
                       lwm2m_data_t ** dataArrayP,
                       lwm2m_object_t * objectP)
{
    actuation_instance_t * targetP;
    int i;

    targetP = (actuation_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0)
    {
        *dataArrayP = lwm2m_data_new(4);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 4;
        (*dataArrayP)[0].id = 5850; // On/Off
        (*dataArrayP)[1].id = 5851; // Dimmer
        (*dataArrayP)[2].id = 5701; // Units
        (*dataArrayP)[3].id = 5750; // Application Type
    }

    for (i = 0; i < *numDataP; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
        case 5850: // On/Off
            lwm2m_data_encode_bool(targetP->onOff, *dataArrayP + i);
            break;
        case 5851: // Dimmer
            lwm2m_data_encode_int(targetP->dimmer, *dataArrayP + i);
            break;
        case 5701: // Units
            lwm2m_data_encode_string(targetP->units, *dataArrayP + i);
            break;
        case 5750: // Application Type
            lwm2m_data_encode_string(targetP->applicationType, *dataArrayP + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_discover(lwm2m_context_t * contextP,
                           uint16_t instanceId,
                           int * numDataP,
                           lwm2m_data_t ** dataArrayP,
                           lwm2m_object_t * objectP)
{
    int i;

    if (*numDataP == 0)
    {
        *numDataP = 4;
        *dataArrayP = lwm2m_data_new(*numDataP);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        (*dataArrayP)[0].id = 5850;
        (*dataArrayP)[1].id = 5851;
        (*dataArrayP)[2].id = 5701;
        (*dataArrayP)[3].id = 5750;
    }
    else
    {
        for (i = 0; i < *numDataP; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case 5850:
            case 5851:
            case 5701:
            case 5750:
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
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
    actuation_instance_t * targetP;
    int i;
    bool state_changed = false;

    targetP = (actuation_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (writeType == LWM2M_WRITE_REPLACE_INSTANCE)
    {
        // Reset all values to defaults for replace operation
        targetP->onOff = false;
        targetP->dimmer = 0;
    }

    for (i = 0; i < numData; i++)
    {
        switch (dataArray[i].id)
        {
        case 5850: // On/Off
            {
                bool newState;
                if (lwm2m_data_decode_bool(dataArray + i, &newState) == 1)
                {
                    if (targetP->onOff != newState)
                    {
                        targetP->onOff = newState;
                        targetP->timestamp = time(NULL);
                        state_changed = true;
                        printf("[Actuation] Instance %d switch %s\n", instanceId, newState ? "ON" : "OFF");
                        
                        // Send SignalK PUT command to control the switch
                        const char* signalk_path = get_signalk_path(instanceId);
                        signalk_put_result_t result = signalk_control_switch(signalk_path, newState);
                        
                        if (result == SIGNALK_PUT_SUCCESS) {
                            printf("[Actuation] ✓ SignalK PUT successful: %s = %s\n", 
                                   signalk_path, newState ? "ON" : "OFF");
                        } else {
                            printf("[Actuation] ✗ SignalK PUT failed: %s (%s)\n", 
                                   signalk_path, signalk_control_error_string(result));
                            // Note: We don't fail the LwM2M operation if SignalK PUT fails
                            // The LwM2M state is updated, but the physical control may not respond
                        }
                    }
                }
                else
                {
                    return COAP_400_BAD_REQUEST;
                }
            }
            break;
        case 5851: // Dimmer
            {
                int64_t dimmerValue;
                if (lwm2m_data_decode_int(dataArray + i, &dimmerValue) == 1)
                {
                    if (dimmerValue < 0) dimmerValue = 0;
                    if (dimmerValue > 100) dimmerValue = 100;
                    
                    if (targetP->dimmer != (int)dimmerValue)
                    {
                        targetP->dimmer = (int)dimmerValue;
                        targetP->timestamp = time(NULL);
                        state_changed = true;
                        printf("[Actuation] Instance %d dimmer set to %d%%\n", instanceId, targetP->dimmer);
                        
                        // Send SignalK PUT command for dimmer control
                        const char* signalk_path = get_signalk_path(instanceId);
                        signalk_put_result_t result = signalk_control_dimmer(signalk_path, targetP->dimmer);
                        
                        if (result == SIGNALK_PUT_SUCCESS) {
                            printf("[Actuation] ✓ SignalK PUT successful: %s dimmer = %d%%\n", 
                                   signalk_path, targetP->dimmer);
                        } else {
                            printf("[Actuation] ✗ SignalK PUT failed: %s dimmer (%s)\n", 
                                   signalk_path, signalk_control_error_string(result));
                            // Note: We don't fail the LwM2M operation if SignalK PUT fails
                        }
                    }
                }
                else
                {
                    return COAP_400_BAD_REQUEST;
                }
            }
            break;
        case 5701: // Units (read-only)
        case 5750: // Application Type (read-only)
            return COAP_405_METHOD_NOT_ALLOWED;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    if (state_changed)
    {
        // Note: Resource change notifications would be handled by the LwM2M server
        // when observing this object instance
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
    // No executable resources in this object
    return COAP_405_METHOD_NOT_ALLOWED;
}

static uint8_t prv_create(lwm2m_context_t * contextP,
                         uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    actuation_instance_t * targetP;
    uint8_t result;

    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(actuation_instance_t));

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
    actuation_instance_t * targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, instanceId, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static void prv_close(lwm2m_object_t * objectP)
{
    LWM2M_LIST_FREE(objectP->instanceList);
    if (objectP->userData != NULL)
    {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
}

/**
 * Initialize actuation instances for marine electrical systems
 */
static void prv_init_instances(lwm2m_object_t * actuationObj)
{
    actuation_instance_t * targetP;
    
    // Instance 0: Navigation Lights
    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (targetP != NULL)
    {
        memset(targetP, 0, sizeof(actuation_instance_t));
        targetP->shortID = 0;
        targetP->onOff = false;
        targetP->dimmer = 0;
        strcpy(targetP->units, "boolean");
        strcpy(targetP->applicationType, "Navigation Lights");
        targetP->timestamp = time(NULL);
        actuationObj->instanceList = LWM2M_LIST_ADD(actuationObj->instanceList, targetP);
        
        // Register with bridge for SignalK integration
        bridge_register(3306, 0, 5850, "electrical.switches.navigation.lights");
    }
    
    // Instance 1: Anchor Light
    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (targetP != NULL)
    {
        memset(targetP, 0, sizeof(actuation_instance_t));
        targetP->shortID = 1;
        targetP->onOff = false;
        targetP->dimmer = 0;
        strcpy(targetP->units, "boolean");
        strcpy(targetP->applicationType, "Anchor Light");
        targetP->timestamp = time(NULL);
        actuationObj->instanceList = LWM2M_LIST_ADD(actuationObj->instanceList, targetP);
        
        bridge_register(3306, 1, 5850, "electrical.switches.anchor.light");
    }
    
    // Instance 2: Bilge Pump
    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (targetP != NULL)
    {
        memset(targetP, 0, sizeof(actuation_instance_t));
        targetP->shortID = 2;
        targetP->onOff = false;
        targetP->dimmer = 0;
        strcpy(targetP->units, "boolean");
        strcpy(targetP->applicationType, "Bilge Pump");
        targetP->timestamp = time(NULL);
        actuationObj->instanceList = LWM2M_LIST_ADD(actuationObj->instanceList, targetP);
        
        bridge_register(3306, 2, 5850, "electrical.switches.bilgePump.main");
    }
    
    // Instance 3: Fresh Water Pump
    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (targetP != NULL)
    {
        memset(targetP, 0, sizeof(actuation_instance_t));
        targetP->shortID = 3;
        targetP->onOff = false;
        targetP->dimmer = 0;
        strcpy(targetP->units, "boolean");
        strcpy(targetP->applicationType, "Fresh Water Pump");
        targetP->timestamp = time(NULL);
        actuationObj->instanceList = LWM2M_LIST_ADD(actuationObj->instanceList, targetP);
        
        bridge_register(3306, 3, 5850, "electrical.switches.freshWaterPump");
    }
    
    // Instance 4: Cabin Lights (with dimmer)
    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (targetP != NULL)
    {
        memset(targetP, 0, sizeof(actuation_instance_t));
        targetP->shortID = 4;
        targetP->onOff = false;
        targetP->dimmer = 50; // Default 50% brightness
        strcpy(targetP->units, "boolean/%");
        strcpy(targetP->applicationType, "Cabin Lights");
        targetP->timestamp = time(NULL);
        actuationObj->instanceList = LWM2M_LIST_ADD(actuationObj->instanceList, targetP);
        
        bridge_register(3306, 4, 5850, "electrical.switches.cabin.lights");
        bridge_register(3306, 4, 5851, "electrical.switches.cabin.dimmer");
    }
    
    // Instance 5: Windlass (Anchor Winch)
    targetP = (actuation_instance_t *)lwm2m_malloc(sizeof(actuation_instance_t));
    if (targetP != NULL)
    {
        memset(targetP, 0, sizeof(actuation_instance_t));
        targetP->shortID = 5;
        targetP->onOff = false;
        targetP->dimmer = 0;
        strcpy(targetP->units, "boolean");
        strcpy(targetP->applicationType, "Windlass");
        targetP->timestamp = time(NULL);
        actuationObj->instanceList = LWM2M_LIST_ADD(actuationObj->instanceList, targetP);
        
        bridge_register(3306, 5, 5850, "electrical.switches.windlass");
    }
    
    printf("[Actuation] Initialized with 6 marine switch control instances\n");
}

lwm2m_object_t * get_actuation_object(void)
{
    lwm2m_object_t * actuationObj;

    actuationObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != actuationObj)
    {
        memset(actuationObj, 0, sizeof(lwm2m_object_t));

        actuationObj->objID = 3306;
        actuationObj->readFunc = prv_read;
        actuationObj->discoverFunc = prv_discover;
        actuationObj->writeFunc = prv_write;
        actuationObj->executeFunc = prv_execute;
        actuationObj->createFunc = prv_create;
        actuationObj->deleteFunc = prv_delete;

    // Do not initialize predefined instances at startup
    // Instances and bridge mappings will be created dynamically when new SignalK data arrives
    printf("[Actuation] Created with no instances. Instances will be added dynamically.\n");
    }

    return actuationObj;
}

void free_actuation_object(lwm2m_object_t * objectP)
{
    if (NULL != objectP)
    {
        LWM2M_LIST_FREE(objectP->instanceList);
        if (objectP->userData != NULL)
        {
            lwm2m_free(objectP->userData);
            objectP->userData = NULL;
        }
        lwm2m_free(objectP);
    }
}

void actuation_update_state(uint16_t instance_id, bool state, lwm2m_context_t *lwm2mH)
{
    // This function would be called from SignalK data updates
    // to synchronize switch states from the vessel's electrical system
    
    printf("[Actuation] State update: Instance %d = %s\n", instance_id, state ? "ON" : "OFF");
    
    // Note: In a full implementation, this would update the actual instance data
    // and trigger notifications to observing servers
}

void actuation_update_dimmer(uint16_t instance_id, int dimmer_value, lwm2m_context_t *lwm2mH)
{
    // This function would be called from SignalK data updates
    // to synchronize dimmer values from the vessel's lighting system
    
    printf("[Actuation] Dimmer update: Instance %d = %d%%\n", instance_id, dimmer_value);
    
    // Note: In a full implementation, this would update the actual instance data
    // and trigger notifications to observing servers
}
