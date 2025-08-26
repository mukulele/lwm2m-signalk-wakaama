// object_signalk.c
// Adapted from Wakatiwai object_generic.c for SignalK WebSocket integration
// TODO: Fill in WebSocket logic and connect to SignalK client

#include "liblwm2m.h"
#include "lwm2mclient.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libwebsockets.h>
#include <cjson/cJSON.h>

// Data structures for object/instance/resource management
// TODO: Adapt as needed for your project

typedef struct {
    uint16_t objInstId;
    // ... add resource data fields as needed ...
    struct generic_obj_instance_t * next;
} generic_obj_instance_t;

// Context for each object
typedef struct {
    uint16_t objectId;
    void * response;
    size_t responseLen;
    struct lws *wsi; // WebSocket instance
} parent_context_t;

// Function prototypes
lwm2m_object_t * get_object(uint16_t objectId);
static uint8_t setup_instance_ids(lwm2m_object_t * objectP);
static uint8_t prv_generic_read_instances(int * numDataP, uint16_t ** instaceIdArrayP, lwm2m_object_t * objectP);
static uint8_t prv_generic_read(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP);
static uint8_t prv_generic_write(uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP);
static uint8_t prv_generic_execute(uint16_t instanceId, uint16_t resourceId, uint8_t * buffer, int length, lwm2m_object_t * objectP);
static uint8_t prv_generic_discover(uint16_t instanceId, int * numDataP, lwm2m_data_t ** dataArrayP, lwm2m_object_t * objectP);
static uint8_t prv_generic_create(uint16_t instanceId, int numData, lwm2m_data_t * dataArray, lwm2m_object_t * objectP);
static uint8_t prv_generic_delete(uint16_t instanceId, lwm2m_object_t * objectP);
static uint8_t request_command(parent_context_t * context, char * cmd, uint8_t * payloadRaw, size_t payloadRawLen);
static uint8_t handle_response(parent_context_t * context, char * cmd);
static void handle_observe_response(/* TODO: args for SignalK message */);
static void lwm2m_data_cp(lwm2m_data_t * dataP, uint8_t * data, uint16_t len);
static size_t lwm2m_write_payload(uint16_t * i, uint8_t * payloadRaw, int numData, lwm2m_data_t * dataArray);
static uint8_t backup_object(lwm2m_object_t * objectP);
static uint8_t restore_object(lwm2m_object_t * objectP);

// Send a JSON message over WebSocket using libwebsockets
void signalk_send_json(struct lws *wsi, const char *json_msg) {
    unsigned char buf[LWS_PRE + 1024];
    size_t len = strlen(json_msg);
    memcpy(&buf[LWS_PRE], json_msg, len);
    lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
}

// Example callback for WebSocket receive (to be integrated with your client)
// This should be called from your libwebsockets callback on LWS_CALLBACK_CLIENT_RECEIVE
void signalk_on_receive(parent_context_t *context, const char *msg) {
    // Parse incoming JSON
    cJSON *json = cJSON_Parse(msg);
    if (!json) return;

    // Example: extract response payload
    cJSON *payload = cJSON_GetObjectItem(json, "payload");
    if (payload && cJSON_IsString(payload)) {
        // TODO: decode base64 if needed, update context->response
        // context->response = ...;
        // context->responseLen = ...;
    }

    // Example: extract status or error code
    cJSON *status = cJSON_GetObjectItem(json, "status");
    if (status && cJSON_IsNumber(status)) {
        // TODO: update context with status
    }

    // Clean up
    cJSON_Delete(json);
}

// Example: Refactored object/resource functions for WebSocket-based SignalK integration

static uint8_t prv_generic_read(uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP) {
    parent_context_t *context = (parent_context_t *)objectP->userData;
    // Build payload for read request
    uint8_t payloadRaw[256];
    size_t payloadRawLen = 0;
    // TODO: Fill payloadRaw with resource IDs, instanceId, etc.
    request_command(context, "read", payloadRaw, payloadRawLen);
    // Response handled asynchronously in signalk_on_receive
    return COAP_NO_ERROR;
}

static uint8_t prv_generic_write(uint16_t instanceId, int numData, lwm2m_data_t *dataArray, lwm2m_object_t *objectP) {
    parent_context_t *context = (parent_context_t *)objectP->userData;
    uint8_t payloadRaw[256];
    size_t payloadRawLen = 0;
    // TODO: Fill payloadRaw with resource data
    request_command(context, "write", payloadRaw, payloadRawLen);
    return COAP_NO_ERROR;
}

static uint8_t prv_generic_create(uint16_t instanceId, int numData, lwm2m_data_t *dataArray, lwm2m_object_t *objectP) {
    parent_context_t *context = (parent_context_t *)objectP->userData;
    uint8_t payloadRaw[256];
    size_t payloadRawLen = 0;
    // TODO: Fill payloadRaw for create
    request_command(context, "create", payloadRaw, payloadRawLen);
    return COAP_NO_ERROR;
}

static uint8_t prv_generic_delete(uint16_t instanceId, lwm2m_object_t *objectP) {
    parent_context_t *context = (parent_context_t *)objectP->userData;
    uint8_t payloadRaw[256];
    size_t payloadRawLen = 0;
    // TODO: Fill payloadRaw for delete
    request_command(context, "delete", payloadRaw, payloadRawLen);
    return COAP_NO_ERROR;
}

// Resource Value Handling & Notification Logic
void signalk_on_receive(parent_context_t *context, const char *msg) {
    cJSON *json = cJSON_Parse(msg);
    if (!json) return;
    cJSON *payload = cJSON_GetObjectItem(json, "payload");
    if (payload && cJSON_IsString(payload)) {
        // TODO: decode base64 if needed, update context->response
        // Example: update LwM2M resource value
        // if (value changed) {
        //     lwm2m_resource_value_changed(lwm2mContext, &uri);
        // }
    }
    cJSON_Delete(json);
}

// TODO: Implement each function, replacing IPC with WebSocket logic
// For example, request_command should send a WebSocket message and wait for a response
// handle_observe_response should be called when a SignalK update is received

// Example usage in request_command
static uint8_t request_command(parent_context_t * context, char * cmd, uint8_t * payloadRaw, size_t payloadRawLen) {
    // Serialize command as JSON
    char json_msg[1024];
    snprintf(json_msg, sizeof(json_msg),
             "{\"request\": \"%s\", \"objectId\": %u, \"payload\": \"%s\"}",
             cmd, context->objectId, base64_encode(payloadRaw, payloadRawLen));

    // Send via WebSocket
    signalk_send_json(context->wsi, json_msg);

    // Response will be handled asynchronously in callback_signalk
    // You may need to set a flag or use a condition variable to wait for the response

    return COAP_NO_ERROR; // Placeholder
}
