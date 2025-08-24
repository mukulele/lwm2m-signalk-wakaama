// Minimal SignalK WebSocket client using libwebsockets
#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>

#include "signalK_observe.h"
#include <cjson/cJSON.h>
#include <stdlib.h>

#define SIGNALK_WS_URL "ws://localhost:3000/signalk/v1/stream"

static int callback_signalk(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Connected to SignalK WebSocket!\n");
            // Subscribe to position and speedOverGround after connection
            signalk_observe_start(wsi, "navigation.position");
            signalk_observe_start(wsi, "navigation.speedOverGround");
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("Received: %.*s\n", (int)len, (char *)in);
            // Example: parse JSON and update value cache
            // cJSON *json = cJSON_Parse((char *)in);
            // if (json) { /* parse and update cache */ cJSON_Delete(json); }
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("Connection error\n");
            break;
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("Connection closed\n");
            break;
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "signalk-protocol",
        callback_signalk,
        0,
        4096,
        0, // id field
        NULL, // user field
        0 // tx_packet_size field
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 }
};

int main(void) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create lws context\n");
        return 1;
    }

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.address = "localhost";
    ccinfo.port = 3000;
    ccinfo.path = "/signalk/v1/stream";
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0;

    struct lws *wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        fprintf(stderr, "WebSocket connection failed\n");
        lws_context_destroy(context);
        return 1;
    }

    // Example: simulate observe start/stop
    // signalk_observe_start("navigation.position");
    // signalk_observe_stop("navigation.position");

    while (lws_service(context, 1000) >= 0) {
        // Event loop
    }

    lws_context_destroy(context);
    return 0;
}
