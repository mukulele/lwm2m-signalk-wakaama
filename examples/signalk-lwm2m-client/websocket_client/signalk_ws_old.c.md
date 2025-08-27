// signalk_ws.c
// Minimal Signal K WebSocket client using libwebsockets.
// Compile with: gcc -o signalk_ws signalk_ws.c -lwebsockets -lcjson -lpthread


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libwebsockets.h>

#include "bridge_object.h"   // contains: void bridge_handle_signalk_delta(const char *json, lwm2m_context_t *ctx);
#include "liblwm2m.h"        // for lwm2m_context_t

// Global pointer to LwM2M context (set by caller)
static lwm2m_context_t *g_lwm2m_ctx = NULL;

// Connection params (customize)
struct ws_params {
    const char *host;      // e.g. "localhost"
    int port;              // e.g. 3000
    const char *path;      // e.g. "/signalk/v1/stream?subscribe=self"
    int use_ssl;           // 0 = ws, 1 = wss
};
static struct ws_params g_params = { "localhost", 3000, "/signalk/v1/stream?subscribe=self", 0 };

// libwebsockets objects
static struct lws_context *g_lws_ctx = NULL;
static struct lws *g_wsi = NULL;
static pthread_t g_thread;
static int g_thread_stop = 0;

// Optional: send a subscription JSON after connect (example)
static const char *subscribe_json =
    "{\"context\":\"vessels.self\",\"subscribe\":[{\"path\":\"navigation.*\",\"period\":1000}],\"replace\":false}";

// this is the per-protocol callback
static int callback_signalk_ws(struct lws *wsi, enum lws_callback_reasons reason,
                               void *user, void *in, size_t len)
{
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        lwsl_user("Signalk WS: connected\n");
        g_wsi = wsi;
        // optionally send a subscribe message (server may accept query-subscribe already)
        if (subscribe_json) {
            size_t msg_len = strlen(subscribe_json);
            // lws_write requires LWS_PRE bytes before payload
            unsigned char *buf = malloc(LWS_PRE + msg_len);
            if (buf) {
                memcpy(&buf[LWS_PRE], subscribe_json, msg_len);
                lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
                free(buf);
            }
        }
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        // in points to received data, len is length (not null terminated)
        // copy and null-terminate then call bridge handler
        {
            char *payload = malloc(len + 1);
            if (!payload) break;
            memcpy(payload, in, len);
            payload[len] = '\0';
            // forward JSON to bridge (bridge must be thread-safe enough)
            if (g_lwm2m_ctx) {
                bridge_handle_signalk_delta(payload, g_lwm2m_ctx);
            } else {
                // still accept locally (or log)
                fprintf(stderr, "bridge: no lwm2m ctx; received: %s\n", payload);
            }
            free(payload);
        }
        break;

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        lwsl_err("Signalk WS: connection error\n");
        g_wsi = NULL;
        break;

    case LWS_CALLBACK_CLOSED:
        lwsl_user("Signalk WS: connection closed\n");
        g_wsi = NULL;
        break;

    default:
        break;
    }

    return 0;
}

// Protocols array
static struct lws_protocols protocols[] = {
    {
        "signalk-protocol",
        callback_signalk_ws,
        0,      // per-session user data size
        0,      // rx buffer size (0 uses default)
    },
    { NULL, NULL, 0, 0 } /* terminator */
};
*/
// Thread running the lws service loop
static void *lws_service_thread(void *arg)
{
    (void)arg;
    while (!g_thread_stop) {
        lws_service(g_lws_ctx, 100); // timeout ms
    }
    return NULL;
}

// helper: start connection
int start_signalk_ws(lwm2m_context_t *lwm2m_ctx, const char *host, int port, const char *path, int use_ssl)
{
    g_lwm2m_ctx = lwm2m_ctx;

    if (host) g_params.host = host;
    if (port) g_params.port = port;
    if (path) g_params.path = path;
    g_params.use_ssl = use_ssl;

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    g_lws_ctx = lws_create_context(&info);
    if (!g_lws_ctx) {
        fprintf(stderr, "Failed to create lws context\n");
        return -1;
    }

    // client connection info
    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = g_lws_ctx;
    ccinfo.address = g_params.host;
    ccinfo.port = g_params.port;
    ccinfo.path = g_params.path;
    ccinfo.host = g_params.host;
    ccinfo.origin = g_params.host;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = g_params.use_ssl ? LCCSCF_USE_SSL : 0;

    if (!lws_client_connect_via_info(&ccinfo)) {
        fprintf(stderr, "Failed to connect to Signal K server %s:%d%s\n", g_params.host, g_params.port, g_params.path);
        lws_context_destroy(g_lws_ctx);
        g_lws_ctx = NULL;
        return -1;
    }

    // start thread for lws_service
    g_thread_stop = 0;
    if (pthread_create(&g_thread, NULL, lws_service_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create lws service thread\n");
        lws_context_destroy(g_lws_ctx);
        g_lws_ctx = NULL;
        return -1;
    }

    return 0;
}

// helper: stop
void stop_signalk_ws(void)
{
    g_thread_stop = 1;
    if (g_thread) {
        pthread_join(g_thread, NULL);
    }
    if (g_lws_ctx) {
        lws_context_destroy(g_lws_ctx);
        g_lws_ctx = NULL;
    }
    g_wsi = NULL;
}

// send a text message (thread-safe via scheduling a writable callback would be better,
// but for simple use-case we do immediate write if connection open).
int signalk_ws_send_text(const char *text)
{
    if (!g_wsi) return -1;
    size_t len = strlen(text);
    unsigned char *buf = malloc(LWS_PRE + len);
    if (!buf) return -1;
    memcpy(&buf[LWS_PRE], text, len);
    int n = lws_write(g_wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT);
    free(buf);
    return (n < 0) ? -1 : 0;
}
