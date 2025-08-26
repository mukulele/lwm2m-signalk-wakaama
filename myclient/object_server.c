#include "liblwm2m.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    struct _server_instance * next; // linked list
    uint16_t instanceId;

    uint16_t shortServerId;
    int lifetime;
    char * binding;
} server_instance_t;

static uint8_t prv_server_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArray,
                               lwm2m_object_t * objectP)
{
    server_instance_t * target = (server_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, instanceId);
    if (target == NULL) return COAP_404_NOT_FOUND;

    if (*numDataP == 0) {
        uint16_t resList[] = {0, 1, 7};
        int nbRes = sizeof(resList)/sizeof(uint16_t);
        *dataArray = lwm2m_data_new(nbRes);
        if (*dataArray == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (int i=0; i<nbRes; i++) (*dataArray)[i].id = resList[i];
    }

    for (int i=0; i<*numDataP; i++) {
        switch ((*dataArray)[i].id) {
        case 0: // Short Server ID
            lwm2m_data_encode_int(target->shortServerId, *dataArray + i);
            break;
        case 1: // Lifetime
            lwm2m_data_encode_int(target->lifetime, *dataArray + i);
            break;
        case 7: // Binding
            lwm2m_data_encode_string(target->binding, *dataArray + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

lwm2m_object_t * get_server_object(void)
{
    lwm2m_object_t * obj = (lwm2m_object_t *)malloc(sizeof(lwm2m_object_t));
    if (NULL == obj) return NULL;
    memset(obj, 0, sizeof(lwm2m_object_t));

    obj->objID = 1;
    obj->readFunc = prv_server_read;

    // Default-Instanz (wird durch Bootstrap überschrieben)
    server_instance_t * inst = (server_instance_t *)malloc(sizeof(server_instance_t));
    if (inst != NULL) {
        memset(inst, 0, sizeof(server_instance_t));
        inst->instanceId = 0;
        inst->shortServerId = 123;    // Dummy
        inst->lifetime = 300;         // Dummy Lifetime
        inst->binding = strdup("U");  // UDP
        obj->instanceList = LWM2M_LIST_ADD(obj->instanceList, inst);
    }

    return obj;
}