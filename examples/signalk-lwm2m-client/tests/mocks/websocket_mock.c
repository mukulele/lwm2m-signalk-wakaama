/**
 * @file mocks/websocket_mock.c
 * @brief WebSocket Mock Implementation
 */

#include "../marine_test_mocks.h"

lwm2m_object_t* test_create_mock_lwm2m_object(uint16_t object_id, uint16_t instance_id)
{
    lwm2m_object_t* obj = malloc(sizeof(lwm2m_object_t));
    if (!obj) return NULL;
    
    memset(obj, 0, sizeof(lwm2m_object_t));
    obj->objID = object_id;
    /* TODO: Initialize object properly */
    
    return obj;
}

void test_destroy_mock_lwm2m_object(lwm2m_object_t* object)
{
    if (object) {
        free(object);
    }
}

bool test_validate_lwm2m_object(lwm2m_object_t* object, uint16_t expected_obj_id)
{
    return (object != NULL && object->objID == expected_obj_id);
}
