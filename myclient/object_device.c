#include "liblwm2m.h"
#include <string.h>
#include <stdlib.h>

static uint8_t prv_device_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArray,
                               lwm2m_object_t * objectP)
{
    if (instanceId != 0) return COAP_404_NOT_FOUND;

    if (*numDataP == 0) {
        uint16_t resList[] = {0,1,2,3};
        int nbRes = sizeof(resList)/sizeof(uint16_t);
        *dataArray = lwm2m_data_new(nbRes);
        if (*dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (int i=0; i<nbRes; i++) (*dataArray)[i].id = resList[i];
    }

    for (int i=0; i<*numDataP; i++) {
        switch ((*dataArray)[i].id) {
        case 0: // Manufacturer
            lwm2m_data_encode_string("MyCompany", *dataArray + i);
            break;
        case 1: // Model Number
            lwm2m_data_encode_string("MyDevice-v1", *dataArray + i);
            break;
        case 2: // Serial Number
            lwm2m_data_encode_string("123456789", *dataArray + i);
            break;
        case 3: // Firmware Version
            lwm2m_data_encode_string("1.0.0", *dataArray + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

lwm2m_object_t * get_object_device(void)
{
    lwm2m_object_t * obj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));
    if (NULL == obj) return NULL;
    memset(obj, 0, sizeof(lwm2m_object_t));

    obj->objID = 3;
    obj->readFunc = prv_device_read;

    // eine Instanz (ID=0)
    lwm2m_list_t * inst = (lwm2m_list_t *)malloc(sizeof(lwm2m_list_t));
    if (inst != NULL) {
        memset(inst, 0, sizeof(lwm2m_list_t));
        inst->id = 0;
        obj->instanceList = inst;
    }

    return obj;
}