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
static uint8_t prv_read(uint16_t instanceId,
                        int *numDataP,
                        lwm2m_data_t **dataArray,
                        lwm2m_object_t *objectP)
{
    sensor_instance_t *inst = (sensor_instance_t *)objectP->userData;
    if (!inst) return COAP_404_NOT_FOUND;

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
static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t *dataArray,
                         lwm2m_object_t *objectP)
{
    sensor_instance_t *inst = (sensor_instance_t *)objectP->userData;
    if (!inst) return COAP_404_NOT_FOUND;

    for (int i = 0; i < numData; i++)
    {
        switch (dataArray[i].id)
        {
        case 5700: // Sensor Value
        {
            char *val = NULL;
            int len = lwm2m_data_decode_string(&dataArray[i], &val);
            if (len > 0 && val)
            {
                strncpy(inst->value, val, sizeof(inst->value));
                inst->value[sizeof(inst->value) - 1] = '\0';
                free(val);

                // Push back into bridge
                bridge_update(inst->path, inst->value);
            }
            break;
        }
        case 5701: // Units
        {
            char *val = NULL;
            int len = lwm2m_data_decode_string(&dataArray[i], &val);
            if (len > 0 && val)
            {
                strncpy(inst->units, val, sizeof(inst->units));
                inst->units[sizeof(inst->units) - 1] = '\0';
                free(val);
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
static uint8_t prv_discover(uint16_t instanceId,
                            int *numDataP,
                            lwm2m_data_t **dataArray,
                            lwm2m_object_t *objectP)
{
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
static uint8_t prv_delete(uint16_t id,
                          lwm2m_object_t *objectP)
{
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

    strncpy(inst->path, path, sizeof(inst->path));
    strncpy(inst->units, units ? units : "", sizeof(inst->units));
    strcpy(inst->value, "0"); // default

    // Add single instance 0
    lwm2m_list_t *instance = lwm2m_list_new(0);
    obj->instanceList = instance;

    obj->readFunc     = prv_read;
    obj->writeFunc    = prv_write;
    obj->discoverFunc = prv_discover;
    obj->deleteFunc   = prv_delete;

    obj->userData = inst;

    // Register with bridge (maps SignalK path → LwM2M resource)
    bridge_register(path, obj, 0, 5700);

    return obj;
}
