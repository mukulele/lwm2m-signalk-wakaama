#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char pattern[64];
    uint16_t objectId;
    uint16_t resourceId;
    char instanceStrategy[16];
} MappingEntry;

#define MAX_MAPPINGS 16
MappingEntry mappingTable[MAX_MAPPINGS];
int mappingCount = 0;

int load_mapping_table(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return -2;

    cJSON *arr = cJSON_GetObjectItem(root, "signalK_to_lwm2m");
    if (!cJSON_IsArray(arr)) {
        cJSON_Delete(root);
        return -3;
    }

    mappingCount = 0;
    cJSON *entry;
    cJSON_ArrayForEach(entry, arr) {
        if (mappingCount >= MAX_MAPPINGS) break;
        cJSON *pattern = cJSON_GetObjectItem(entry, "pattern");
        cJSON *objectId = cJSON_GetObjectItem(entry, "objectId");
        cJSON *resourceId = cJSON_GetObjectItem(entry, "resourceId");
        cJSON *instanceStrategy = cJSON_GetObjectItem(entry, "instanceStrategy");
        if (cJSON_IsString(pattern) && cJSON_IsNumber(objectId) && cJSON_IsNumber(resourceId) && cJSON_IsString(instanceStrategy)) {
            strncpy(mappingTable[mappingCount].pattern, pattern->valuestring, sizeof(mappingTable[mappingCount].pattern)-1);
            mappingTable[mappingCount].objectId = (uint16_t)objectId->valuedouble;
            mappingTable[mappingCount].resourceId = (uint16_t)resourceId->valuedouble;
            strncpy(mappingTable[mappingCount].instanceStrategy, instanceStrategy->valuestring, sizeof(mappingTable[mappingCount].instanceStrategy)-1);
            mappingCount++;
        }
    }
    cJSON_Delete(root);
    return mappingCount;
}
#include <libwebsockets.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "bridge_object.h"
#include "signalk_subscriptions.h"

// Authentication state management (consolidated from signalk_auth)
typedef struct {
    bool enabled;
    char username[64];
    char password[64];
    char token[512];
    time_t token_expires;
    bool authenticated;
    char request_id[32];
    uint32_t request_counter;
} auth_context_t;

static auth_context_t g_auth = {0};

static struct lws_context *context;
static struct lws *wsi;
static pthread_t ws_thread;
static int running = 1;
static int subscription_sent = 0;
static int auth_flow_started = 0;

// ============================================================================
// Consolidated Authentication Functions
// ============================================================================

static void generate_request_id(char *request_id, size_t max_len) {
    g_auth.request_counter++;
    snprintf(request_id, max_len, "lwm2m-auth-%u-%lu", 
             g_auth.request_counter, (unsigned long)time(NULL));
}

static bool signalk_auth_is_enabled(void) {
    return g_auth.enabled;
}

static bool signalk_auth_is_authenticated(void) {
    if (!g_auth.enabled) return true; // No auth required
    return g_auth.authenticated && (g_auth.token[0] != '\0') && (time(NULL) < g_auth.token_expires);
}

static size_t signalk_auth_generate_login_message(char *buffer, size_t buffer_size) {
    if (!buffer || !g_auth.enabled) return 0;
    
    generate_request_id(g_auth.request_id, sizeof(g_auth.request_id));
    
    int result = snprintf(buffer, buffer_size,
        "{"
        "\"requestId\":\"%s\","
        "\"login\":{"
        "\"username\":\"%s\","
        "\"password\":\"%s\""
        "}"
        "}",
        g_auth.request_id, g_auth.username, g_auth.password
    );
    
    if (result > 0 && (size_t)result < buffer_size) {
        printf("[SignalK Auth] Generated login message for user: %s\n", g_auth.username);
        return (size_t)result;
    }
    return 0;
}

static bool signalk_auth_process_response(const char *json_response) {
    if (!json_response || !g_auth.enabled) return false;
    
    cJSON *root = cJSON_Parse(json_response);
    if (!root) return false;
    
    // Check if this is our auth response
    cJSON *request_id_obj = cJSON_GetObjectItem(root, "requestId");
    if (!request_id_obj || strcmp(cJSON_GetStringValue(request_id_obj), g_auth.request_id) != 0) {
        cJSON_Delete(root);
        return false;
    }
    
    // Check result code
    cJSON *result_obj = cJSON_GetObjectItem(root, "result");
    if (!result_obj || cJSON_GetNumberValue(result_obj) != 200) {
        printf("[SignalK Auth] Authentication failed\n");
        g_auth.authenticated = false;
        cJSON_Delete(root);
        return false;
    }
    
    // Extract token
    cJSON *login_obj = cJSON_GetObjectItem(root, "login");
    cJSON *token_obj = cJSON_GetObjectItem(login_obj, "token");
    if (!token_obj) {
        cJSON_Delete(root);
        return false;
    }
    
    const char *token = cJSON_GetStringValue(token_obj);
    if (!token || strlen(token) >= sizeof(g_auth.token)) {
        cJSON_Delete(root);
        return false;
    }
    
    // Get TTL
    cJSON *ttl_obj = cJSON_GetObjectItem(login_obj, "timeToLive");
    int ttl = ttl_obj ? cJSON_GetNumberValue(ttl_obj) : 3600;
    
    // Store authentication data
    strncpy(g_auth.token, token, sizeof(g_auth.token) - 1);
    g_auth.token[sizeof(g_auth.token) - 1] = '\0';
    g_auth.token_expires = time(NULL) + ttl;
    g_auth.authenticated = true;
    
    printf("[SignalK Auth] Successfully authenticated! Token expires in %d seconds\n", ttl);
    
    cJSON_Delete(root);
    return true;
}

static bool signalk_auth_init(const char *username, const char *password) {
    if (!username || !password) {
        g_auth.enabled = false;
        return true;
    }
    
    g_auth.enabled = true;
    strncpy(g_auth.username, username, sizeof(g_auth.username) - 1);
    strncpy(g_auth.password, password, sizeof(g_auth.password) - 1);
    g_auth.authenticated = false;
    g_auth.token[0] = '\0';
    g_auth.token_expires = 0;
    g_auth.request_counter = 0;
    
    printf("[SignalK Auth] Initialized - Authentication ENABLED for user: %s\n", username);
    return true;
}

static void signalk_auth_cleanup(void) {
    memset(&g_auth, 0, sizeof(g_auth));
    printf("[SignalK Auth] Cleanup completed\n");
}

// ============================================================================
// WebSocket Callback Function
// ============================================================================

static int callback_signalk(struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    static int mapping_initialized = 0;
    if (!mapping_initialized) {
        int loaded = load_mapping_table("settings.json");
        printf("[SignalK] Loaded %d mapping entries from settings.json\n", loaded);
        mapping_initialized = 1;
    }
    switch (reason)
    {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("[SignalK] Connected to server\n");
        subscription_sent = 0;
        auth_flow_started = 0;
        lws_callback_on_writable(wsi);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        // Handle authentication flow first if enabled
        if (signalk_auth_is_enabled() && !signalk_auth_is_authenticated() && !auth_flow_started) {
            char login_msg[512];
            size_t login_len = signalk_auth_generate_login_message(login_msg, sizeof(login_msg));
            
            if (login_len > 0) {
                unsigned char buf[LWS_PRE + login_len];
                memcpy(&buf[LWS_PRE], login_msg, login_len);
                
                int n = lws_write(wsi, &buf[LWS_PRE], login_len, LWS_WRITE_TEXT);
                if (n < 0) {
                    printf("[SignalK Auth] Failed to send login message\n");
                    return -1;
                }
                printf("[SignalK Auth] Login message sent\n");
                auth_flow_started = 1;
            }
        }
        // Send subscription if authenticated (or auth disabled)
        else if (!subscription_sent && (!signalk_auth_is_enabled() || signalk_auth_is_authenticated())) {
            printf("[SignalK] Sending subscription\n");
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
        if (json)
        {
            // Check if this is an authentication response
            if (signalk_auth_is_enabled() && signalk_auth_process_response(msg)) {
                printf("[SignalK Auth] Authentication response processed\n");
                // After successful authentication, trigger subscription
                if (signalk_auth_is_authenticated() && !subscription_sent) {
                    lws_callback_on_writable(wsi);
                }
                cJSON_Delete(json);
                free(msg);
                break;
            }
            
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
                                // --- Generalized dynamic registration logic ---
                                // Find mapping entry for this path
                                MappingEntry *found = NULL;
                                for (int m = 0; m < mappingCount; m++) {
                                    // Simple wildcard match: pattern ends with '*' matches prefix
                                    size_t plen = strlen(mappingTable[m].pattern);
                                    if (mappingTable[m].pattern[plen-1] == '*') {
                                        if (strncmp(path->valuestring, mappingTable[m].pattern, plen-1) == 0) {
                                            found = &mappingTable[m];
                                            break;
                                        }
                                    } else if (strcmp(path->valuestring, mappingTable[m].pattern) == 0) {
                                        found = &mappingTable[m];
                                        break;
                                    }
                                }
                                if (!found) {
                                    printf("[SignalK] No mapping for path: %s\n", path->valuestring);
                                    return 0;
                                }

                                // Instance ID extraction (suffix strategy)
                                uint16_t instId = 0;
                                if (strcmp(found->instanceStrategy, "suffix") == 0) {
                                    const char *lastdot = strrchr(path->valuestring, '.');
                                    if (lastdot && *(lastdot+1)) {
                                        instId = (uint16_t)atoi(lastdot+1); // fallback: use numeric suffix
                                    }
                                }

                                // Check if mapping exists
                                int mapping_exists = 0;
                                for (int i = 0; i < registry_count; i++) {
                                    if (strcmp(registry[i].signalK_path, path->valuestring) == 0) {
                                        mapping_exists = 1;
                                        break;
                                    }
                                }
                                if (!mapping_exists) {
                                    // Create new instance in the correct object
                                    lwm2m_object_t *obj = NULL;
                                    extern lwm2m_object_t *energyObj, *genericSensorObj, *locationObj, *powerMeasurementObj;
                                    switch (found->objectId) {
                                        case 3331: obj = energyObj; break;
                                        case 3300: obj = genericSensorObj; break;
                                        case 3336: obj = powerMeasurementObj; break;
                                        case 6:    obj = locationObj; break;
                                        // Add more as needed
                                    }
                                    if (obj && obj->createFunc) {
                                        obj->createFunc(NULL, instId, 0, NULL, obj);
                                        printf("[SignalK] Created instance %d for object %u, path %s\n", instId, found->objectId, path->valuestring);
                                    }
                                    bridge_register(found->objectId, instId, found->resourceId, path->valuestring);
                                }
                                // --- End generalized dynamic registration logic ---

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

    // Initialize authentication if enabled
    char token_file[512];
    
    // Determine path to token.json relative to settings.json directory
    const char *last_slash = strrchr(config_file, '/');
    if (last_slash) {
        // settings.json is in a subdirectory - token.json should be in same directory
        size_t dir_len = last_slash - config_file + 1;
        strncpy(token_file, config_file, dir_len);
        token_file[dir_len] = '\0';
        strcat(token_file, "token.json");
    } else {
        // settings.json is in current directory
        strcpy(token_file, "token.json");
    }
    
    // Try to load authentication configuration
    if (access(token_file, F_OK) == 0) {
        // Load authentication config from token.json
        FILE *fp = fopen(token_file, "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long length = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            
            char *token_content = malloc(length + 1);
            if (token_content) {
                fread(token_content, 1, length, fp);
                token_content[length] = '\0';
                
                cJSON *token_json = cJSON_Parse(token_content);
                if (token_json) {
                    cJSON *auth_obj = cJSON_GetObjectItem(token_json, "authentication");
                    if (auth_obj) {
                        cJSON *enabled = cJSON_GetObjectItem(auth_obj, "enabled");
                        cJSON *username = cJSON_GetObjectItem(auth_obj, "username");
                        cJSON *password = cJSON_GetObjectItem(auth_obj, "password");
                        
                        if (enabled && cJSON_IsTrue(enabled) && 
                            username && cJSON_IsString(username) &&
                            password && cJSON_IsString(password)) {
                            
                            // Initialize authentication with username/password
                            if (!signalk_auth_init(cJSON_GetStringValue(username), 
                                                 cJSON_GetStringValue(password))) {
                                printf("[SignalK Auth] Warning: Failed to initialize authentication\n");
                            }
                        } else {
                            // Authentication disabled or incomplete config
                            signalk_auth_init(NULL, NULL);
                        }
                    }
                    cJSON_Delete(token_json);
                }
                free(token_content);
            }
            fclose(fp);
        }
    } else {
        printf("[SignalK Auth] No token.json found - authentication disabled\n");
        signalk_auth_init(NULL, NULL);
    }

    subscription_sent = 0;
    auth_flow_started = 0;

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
    ccinfo.address = signalk_server_config ? signalk_server_config->host : "127.0.0.1";
    ccinfo.port = signalk_server_config ? signalk_server_config->port : 3000;
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
    
    // Cleanup authentication module
    signalk_auth_cleanup();
    
    printf("[SignalK] WebSocket client stopped\n");
}

bool signalk_ws_is_connected(void) {
    return (running && context && wsi);
}
