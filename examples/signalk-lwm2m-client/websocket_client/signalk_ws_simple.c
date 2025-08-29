#include <libwebsockets.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "bridge_object.h"
#include "signalk_subscriptions.h"

static struct lws_context *context;
static struct lws *wsi;
static pthread_t ws_thread;
static int running = 1;
static int subscription_sent = 0;

static int callback_signalk(struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("[SignalK] Connected to server - sending subscription\n");
        subscription_sent = 0;
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        if (!subscription_sent) {
            char* subscription_json = NULL;
            if (signalk_create_subscription_message(&subscription_json)) {
                size_t sub_len = strlen(subscription_json);
                unsigned char buf[LWS_PRE + sub_len];
                memcpy(&buf[LWS_PRE], subscription_json, sub_len);
                
                int n = lws_write(wsi, &buf[LWS_PRE], sub_len, LWS_WRITE_TEXT);
                if (n < 0) {
                    printf("[SignalK] Failed to send subscription\n");
                    free(subscription_json);
                    return -1;
                }
                printf("[SignalK] Subscription sent (%zu bytes)\n", sub_len);
                subscription_sent = 1;
                free(subscription_json);
            } else {
                printf("[SignalK] Failed to create subscription message\n");
                return -1;
            }
        }
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
        char *msg = strndup((const char *)in, len);
        if (!msg) break;

        cJSON *json = cJSON_Parse(msg);
        free(msg);

        if (json)
        {
            // Signal K "delta" message has an "updates" array
            cJSON *updates = cJSON_GetObjectItem(json, "updates");
            if (cJSON_IsArray(updates))
            {
                cJSON *upd;
                cJSON_ArrayForEach(upd, updates)
                {
                    cJSON *values = cJSON_GetObjectItem(upd, "values");
                    if (cJSON_IsArray(values))
                    {
                        cJSON *val;
                        cJSON_ArrayForEach(val, values)
                        {
                            cJSON *path = cJSON_GetObjectItem(val, "path");
                            cJSON *value = cJSON_GetObjectItem(val, "value");
                            if (cJSON_IsString(path) && value)
                            {
                                char valbuf[64];

                                if (cJSON_IsNumber(value))
                                {
                                    snprintf(valbuf, sizeof(valbuf), "%.3f", value->valuedouble);
                                    bridge_update(path->valuestring, valbuf);
                                    printf("[SignalK] %s = %s\n", path->valuestring, valbuf);
                                }
                                else if (cJSON_IsString(value))
                                {
                                    bridge_update(path->valuestring, value->valuestring);
                                    printf("[SignalK] %s = %s\n", path->valuestring, value->valuestring);
                                }
                                else if (cJSON_IsBool(value))
                                {
                                    const char *bool_str = cJSON_IsTrue(value) ? "true" : "false";
                                    bridge_update(path->valuestring, bool_str);
                                    printf("[SignalK] %s = %s\n", path->valuestring, bool_str);
                                }
                            }
                        }
                    }
                }
            }
            cJSON_Delete(json);
        }
        break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        printf("[SignalK] Connection error\n");
        wsi = NULL;
        break;

    case LWS_CALLBACK_CLOSED:
        printf("[SignalK] Disconnected\n");
        wsi = NULL;
        break;

    default:
        break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "signalk-ws-protocol",
        callback_signalk,
        0,
        4096,
    },
    { NULL, NULL, 0, 0 } // terminator
};

static void *ws_loop(void *arg)
{
    printf("[SignalK] WebSocket service loop started\n");
    
    while (running && context)
    {
        if (lws_service(context, 100) < 0) {
            printf("[SignalK] WebSocket service error\n");
            break;
        }
        usleep(10000); // 10ms
    }
    
    printf("[SignalK] WebSocket service loop ended\n");
    return NULL;
}

int signalk_ws_start(const char *server, int port, const char *settings_file)
{
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;

    // Load configuration 
    const char *config_file = settings_file ? settings_file : "settings.json";
    if (!signalk_load_config_from_file(config_file)) {
        printf("[SignalK] Error: Failed to load %s\n", config_file);
        return -1;
    }

    subscription_sent = 0;

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "[SignalK] Failed to create context\n");
        return -1;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = server;
    ccinfo.port = port;
    ccinfo.path = "/signalk/v1/stream?subscribe=none";
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        fprintf(stderr, "[SignalK] Failed to connect to %s:%d\n", server, port);
        lws_context_destroy(context);
        context = NULL;
        return -1;
    }

    running = 1;
    if (pthread_create(&ws_thread, NULL, ws_loop, NULL) != 0) {
        fprintf(stderr, "[SignalK] Failed to create thread\n");
        lws_context_destroy(context);
        context = NULL;
        wsi = NULL;
        return -1;
    }
    
    return 0;
}

void signalk_ws_stop(void)
{
    if (running) {
        printf("[SignalK] Stopping WebSocket client...\n");
        running = 0;
        
        usleep(100000); // 100ms
        
        if (ws_thread) {
            pthread_join(ws_thread, NULL);
            ws_thread = 0;
        }
    }
    
    if (context) {
        lws_context_destroy(context);
        context = NULL;
    }
    
    printf("[SignalK] WebSocket client stopped\n");
}
