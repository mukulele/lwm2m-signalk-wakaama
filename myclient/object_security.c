#include "liblwm2m.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    struct _security_instance * next; // linked list
    uint16_t instanceId;

    char * uri;
    bool isBootstrap;
    uint8_t securityMode; // 0 = NoSec
} security_instance_t;

// --- Callback: READ ---
static uint8_t prv_security_read(uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_data_t ** dataArray,
                                 lwm2m_object_t * objectP)
{
    security_instance_t * target = (security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, instanceId);
    if (target == NULL) return COAP_404_NOT_FOUND;

    if (*numDataP == 0) {
        uint16_t resList[] = {0,1,2};
        int nbRes = sizeof(resList)/sizeof(uint16_t);
        *dataArray = lwm2m_data_new(nbRes);
        if (*dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (int i = 0; i < nbRes; i++) {
            (*dataArray)[i].id = resList[i];
        }
    }

    for (int i = 0; i < *numDataP; i++) {
        switch ((*dataArray)[i].id) {
        case 0: // URI
            lwm2m_data_encode_string(target->uri, *dataArray + i);
            break;
        case 1: // Bootstrap Flag
            lwm2m_data_encode_bool(target->isBootstrap, *dataArray + i);
            break;
        case 2: // Security Mode
            lwm2m_data_encode_int(target->securityMode, *dataArray + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

lwm2m_object_t * get_security_object(void)
{
    lwm2m_object_t * obj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));
    if (NULL == obj) return NULL;
    memset(obj, 0, sizeof(lwm2m_object_t));

    obj->objID = 0;

    obj->readFunc = prv_security_read;

    // Bootstrap-Instanz anlegen
    security_instance_t * inst = (security_instance_t *)malloc(sizeof(security_instance_t));
    if (inst == NULL) {
        free(obj);
        return NULL;
    }
    memset(inst, 0, sizeof(security_instance_t));

    inst->instanceId = 0;
    inst->uri = strdup("coap://lwm2m.os.1nce.com:5683");
    inst->isBootstrap = true;
    inst->securityMode = 0; // NoSec

    obj->instanceList = LWM2M_LIST_ADD(obj->instanceList, inst);

    return obj;
}