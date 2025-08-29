/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Benjamin Cabé - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Christian Renz - Please refer to git log
 *    Ricky Liu - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>
 Bosch Software Innovations GmbH - Please refer to git log

*/

#include "lwm2mclient.h"
#include "commandline.h"
#include "liblwm2m.h"
#include "bridge_object.h"
#include "object_power_measurement.h"
#include "object_energy.h"
#include "object_actuation.h"
#include "../websocket_client/signalk_ws.h"
#ifdef WITH_TINYDTLS
#include "tinydtls/connection.h"
#else
#include "udp/connection.h"
#endif

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "wakaama_logger.h"  // Include the header file for our logger

#define MAX_PACKET_SIZE 2048
#define DEFAULT_SERVER_IPV6 "[::1]"
#define DEFAULT_SERVER_IPV4 "127.0.0.1"

int g_reboot = 0;
static int g_quit = 0;

#define OBJ_COUNT 10
lwm2m_object_t *objArray[OBJ_COUNT];

// only backup security and server objects
#define BACKUP_OBJECT_COUNT 2
lwm2m_object_t *backupObjectArray[BACKUP_OBJECT_COUNT];

typedef struct {
    lwm2m_object_t *securityObjP;
    lwm2m_object_t *serverObject;
    int sock;
#ifdef WITH_TINYDTLS
    lwm2m_dtls_connection_t *connList;
    lwm2m_context_t *lwm2mH;
#else
    lwm2m_connection_t *connList;
#endif
    int addressFamily;
} client_data_t;

static void prv_quit(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    /* unused parameters */
    (void)lwm2mH;
    (void)buffer;
    (void)user_data;

    g_quit = 1;
}

void handle_sigint(int signum) { 
    fprintf(stdout, "\n[SIGNAL] Received SIGINT, shutting down gracefully...\n");
    fflush(stdout);
    g_quit = 2; 
}

void handle_value_changed(lwm2m_context_t *lwm2mH, lwm2m_uri_t *uri, const char *value, size_t valueLength) {
    lwm2m_object_t *object = (lwm2m_object_t *)LWM2M_LIST_FIND(lwm2mH->objectList, uri->objectId);

    if (NULL != object) {
        if (object->writeFunc != NULL) {
            lwm2m_data_t *dataP;
            int result;

            dataP = lwm2m_data_new(1);
            if (dataP == NULL) {
                fprintf(stderr, "Internal allocation failure !\n");
                return;
            }
            dataP->id = uri->resourceId;

#ifndef LWM2M_VERSION_1_0
            if (LWM2M_URI_IS_SET_RESOURCE_INSTANCE(uri)) {
                lwm2m_data_t *subDataP = lwm2m_data_new(1);
                if (subDataP == NULL) {
                    fprintf(stderr, "Internal allocation failure !\n");
                    lwm2m_data_free(1, dataP);
                    return;
                }
                subDataP->id = uri->resourceInstanceId;
                lwm2m_data_encode_nstring(value, valueLength, subDataP);
                lwm2m_data_encode_instances(subDataP, 1, dataP);
            } else
#endif
            {
                lwm2m_data_encode_nstring(value, valueLength, dataP);
            }

            result = object->writeFunc(lwm2mH, uri->instanceId, 1, dataP, object, LWM2M_WRITE_PARTIAL_UPDATE);
            if (COAP_405_METHOD_NOT_ALLOWED == result) {
                switch (uri->objectId) {
                case LWM2M_DEVICE_OBJECT_ID:
                    result = device_change(dataP, object);
                    break;
                default:
                    break;
                }
            }

            if (COAP_204_CHANGED != result) {
                fprintf(stderr, "Failed to change value!\n");
            } else {
                fprintf(stderr, "value changed!\n");
                lwm2m_resource_value_changed(lwm2mH, uri);
            }
            lwm2m_data_free(1, dataP);
            return;
        } else {
            fprintf(stderr, "write not supported for specified resource!\n");
        }
        return;
    } else {
        fprintf(stderr, "Object not found !\n");
    }
}

#ifdef WITH_TINYDTLS
void *lwm2m_connect_server(uint16_t secObjInstID, void *userData) {
    client_data_t *dataP;
    lwm2m_list_t *instance;
    lwm2m_dtls_connection_t *newConnP = NULL;
    dataP = (client_data_t *)userData;
    lwm2m_object_t *securityObj = dataP->securityObjP;

    instance = LWM2M_LIST_FIND(dataP->securityObjP->instanceList, secObjInstID);
    if (instance == NULL)
        return NULL;

    newConnP = lwm2m_connection_create(dataP->connList, dataP->sock, securityObj, instance->id, dataP->lwm2mH,
                                       dataP->addressFamily);
    if (newConnP == NULL) {
        fprintf(stderr, "Connection creation failed.\n");
        return NULL;
    }

    dataP->connList = newConnP;
    return (void *)newConnP;
}
#else
void *lwm2m_connect_server(uint16_t secObjInstID, void *userData) {
    client_data_t *dataP;
    char *uri;
    char *host;
    char *port;
    lwm2m_connection_t *newConnP = NULL;

    dataP = (client_data_t *)userData;

    uri = get_server_uri(dataP->securityObjP, secObjInstID);

    if (uri == NULL)
        return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    if (0 == strncmp(uri, "coaps://", strlen("coaps://"))) {      // NOSONAR
        host = uri + strlen("coaps://");                          // NOSONAR
    } else if (0 == strncmp(uri, "coap://", strlen("coap://"))) { // NOSONAR
        host = uri + strlen("coap://");                           // NOSONAR
    } else {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL)
        goto exit;
    // remove brackets
    if (host[0] == '[') {
        host++;
        if (*(port - 1) == ']') {
            *(port - 1) = 0;
        } else
            goto exit;
    }
    // split strings
    *port = 0;
    port++;

    fprintf(stderr, "Opening connection to server at %s:%s\r\n", host, port);
    newConnP = lwm2m_connection_create(dataP->connList, dataP->sock, host, port, dataP->addressFamily);
    if (newConnP == NULL) {
        fprintf(stderr, "Connection creation failed.\r\n");
    } else {
        dataP->connList = newConnP;
    }

exit:
    lwm2m_free(uri);
    return (void *)newConnP;
}
#endif

void lwm2m_close_connection(void *sessionH, void *userData) {
    client_data_t *app_data;
#ifdef WITH_TINYDTLS
    lwm2m_dtls_connection_t *targetP;
#else
    lwm2m_connection_t *targetP;
#endif

    app_data = (client_data_t *)userData;
#ifdef WITH_TINYDTLS
    targetP = (lwm2m_dtls_connection_t *)sessionH;
#else
    targetP = (lwm2m_connection_t *)sessionH;
#endif

    if (targetP == app_data->connList) {
        app_data->connList = targetP->next;
        lwm2m_free(targetP);
    } else {
#ifdef WITH_TINYDTLS
        lwm2m_dtls_connection_t *parentP;
#else
        lwm2m_connection_t *parentP;
#endif

        parentP = app_data->connList;
        while (parentP != NULL && parentP->next != targetP) {
            parentP = parentP->next;
        }
        if (parentP != NULL) {
            parentP->next = targetP->next;
            lwm2m_free(targetP);
        }
    }
}

static void prv_output_servers(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_server_t *targetP;

    /* unused parameter */
    (void)user_data;

    targetP = lwm2mH->bootstrapServerList;

    if (lwm2mH->bootstrapServerList == NULL) {
        fprintf(stdout, "No Bootstrap Server.\r\n");
    } else {
        fprintf(stdout, "Bootstrap Servers:\r\n");
        for (targetP = lwm2mH->bootstrapServerList; targetP != NULL; targetP = targetP->next) {
            fprintf(stdout, " - Security Object ID %d", targetP->secObjInstID);
            fprintf(stdout, "\tHold Off Time: %lu s", (unsigned long)targetP->lifetime);
            fprintf(stdout, "\tstatus: ");
            switch (targetP->status) {
            case STATE_DEREGISTERED:
                fprintf(stdout, "DEREGISTERED\r\n");
                break;
            case STATE_BS_HOLD_OFF:
                fprintf(stdout, "CLIENT HOLD OFF\r\n");
                break;
            case STATE_BS_INITIATED:
                fprintf(stdout, "BOOTSTRAP INITIATED\r\n");
                break;
            case STATE_BS_PENDING:
                fprintf(stdout, "BOOTSTRAP PENDING\r\n");
                break;
            case STATE_BS_FINISHED:
                fprintf(stdout, "BOOTSTRAP FINISHED\r\n");
                break;
            case STATE_BS_FAILED:
                fprintf(stdout, "BOOTSTRAP FAILED\r\n");
                break;
            default:
                fprintf(stdout, "INVALID (%d)\r\n", (int)targetP->status);
            }
        }
    }

    if (lwm2mH->serverList == NULL) {
        fprintf(stdout, "No LwM2M Server.\r\n");
    } else {
        fprintf(stdout, "LWM2M Servers:\r\n");
        for (targetP = lwm2mH->serverList; targetP != NULL; targetP = targetP->next) {
            fprintf(stdout, " - Server ID %d", targetP->shortID);
            fprintf(stdout, "\tstatus: ");
            switch (targetP->status) {
            case STATE_DEREGISTERED:
                fprintf(stdout, "DEREGISTERED\r\n");
                break;
            case STATE_REG_PENDING:
                fprintf(stdout, "REGISTRATION PENDING\r\n");
                break;
            case STATE_REGISTERED:
                fprintf(stdout, "REGISTERED\tlocation: \"%s\"\tLifetime: %lus\r\n", targetP->location,
                        (unsigned long)targetP->lifetime);
                break;
            case STATE_REG_UPDATE_PENDING:
                fprintf(stdout, "REGISTRATION UPDATE PENDING\r\n");
                break;
            case STATE_DEREG_PENDING:
                fprintf(stdout, "DEREGISTRATION PENDING\r\n");
                break;
            case STATE_REG_FAILED:
                fprintf(stdout, "REGISTRATION FAILED\r\n");
                break;
            default:
                fprintf(stdout, "INVALID (%d)\r\n", (int)targetP->status);
            }
        }
    }
}

static void prv_change(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_uri_t uri;
    char *end = NULL;
    int result;

    /* unused parameter */
    (void)user_data;

    end = get_end_of_arg(buffer);
    if (end[0] == 0)
        goto syntax_error;

    result = lwm2m_stringToUri(buffer, end - buffer, &uri);
    if (result == 0)
        goto syntax_error;

    buffer = get_next_arg(end, &end);

    if (buffer[0] == 0) {
        fprintf(stdout, "[OBSERVE] Triggering resource value change for /%d/%d/%d\r\n", 
                uri.objectId, uri.instanceId, uri.resourceId);
        lwm2m_resource_value_changed(lwm2mH, &uri);
    } else {
        handle_value_changed(lwm2mH, &uri, buffer, end - buffer);
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !\n");
}

static void prv_object_list(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_object_t *objectP;

    /* unused parameter */
    (void)user_data;

    for (objectP = lwm2mH->objectList; objectP != NULL; objectP = objectP->next) {
        if (objectP->instanceList == NULL) {
            fprintf(stdout, "/%d ", objectP->objID);
        } else {
            lwm2m_list_t *instanceP;

            for (instanceP = objectP->instanceList; instanceP != NULL; instanceP = instanceP->next) {
                fprintf(stdout, "/%d/%d  ", objectP->objID, instanceP->id);
            }
        }
        fprintf(stdout, "\r\n");
    }
}

static void prv_instance_dump(lwm2m_context_t *lwm2mH, lwm2m_object_t *objectP, uint16_t id) {
    int numData;
    lwm2m_data_t *dataArray;
    uint16_t res;

    numData = 0;
    res = objectP->readFunc(lwm2mH, id, &numData, &dataArray, objectP);
    if (res != COAP_205_CONTENT) {
        printf("Error ");
        print_status(stdout, res);
        printf("\r\n");
        return;
    }

    dump_tlv(stdout, numData, dataArray, 0);
}

static void prv_object_dump(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_uri_t uri;
    char *end = NULL;
    int result;
    lwm2m_object_t *objectP;

    /* unused parameter */
    (void)user_data;

    end = get_end_of_arg(buffer);
    if (end[0] == 0)
        goto syntax_error;

    result = lwm2m_stringToUri(buffer, end - buffer, &uri);
    if (result == 0)
        goto syntax_error;
    if (LWM2M_URI_IS_SET_RESOURCE(&uri))
        goto syntax_error;

    objectP = (lwm2m_object_t *)LWM2M_LIST_FIND(lwm2mH->objectList, uri.objectId);
    if (objectP == NULL) {
        fprintf(stdout, "Object not found.\n");
        return;
    }

    if (LWM2M_URI_IS_SET_INSTANCE(&uri)) {
        prv_instance_dump(lwm2mH, objectP, uri.instanceId);
    } else {
        lwm2m_list_t *instanceP;

        for (instanceP = objectP->instanceList; instanceP != NULL; instanceP = instanceP->next) {
            fprintf(stdout, "Instance %d:\r\n", instanceP->id);
            prv_instance_dump(lwm2mH, objectP, instanceP->id);
            fprintf(stdout, "\r\n");
        }
    }

    return;

syntax_error:
    fprintf(stdout, "Syntax error !\n");
}

static void prv_update(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    /* unused parameter */
    (void)user_data;

    if (buffer[0] == 0)
        goto syntax_error;

    uint16_t serverId = (uint16_t)atoi(buffer);
    int res = lwm2m_update_registration(lwm2mH, serverId, false);
    if (res != 0) {
        fprintf(stdout, "Registration update error: ");
        print_status(stdout, res);
        fprintf(stdout, "\r\n");
    }
    return;

syntax_error:
    fprintf(stdout, "Syntax error !\n");
}

#ifndef LWM2M_VERSION_1_0
static void prv_send(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_uri_t uri;
    lwm2m_uri_t *uris = NULL;
    size_t uriCount = 0;
    char *tmp;
    char *end = NULL;
    int result;
    uint16_t serverId;

    /* unused parameter */
    (void)user_data;

    if (buffer[0] == 0)
        goto syntax_error;

    result = atoi(buffer);
    if (result < 0 || result > LWM2M_MAX_ID)
        goto syntax_error;
    serverId = (uint16_t)result;

    tmp = buffer;
    do {
        tmp = get_next_arg(tmp, &end);
        if (tmp[0] == 0)
            goto syntax_error;

        result = lwm2m_stringToUri(tmp, end - tmp, &uri);
        if (result == 0)
            goto syntax_error;
        uriCount++;
    } while (!check_end_of_args(end));

    uris = lwm2m_malloc(uriCount * sizeof(lwm2m_uri_t));
    if (uris != NULL) {
        size_t i;
        for (i = 0; i < uriCount; i++) {
            buffer = get_next_arg(buffer, &end);
            if (buffer[0] == 0)
                goto syntax_error;

            result = lwm2m_stringToUri(buffer, end - buffer, uris + i);
            if (result == 0)
                goto syntax_error;
        }

        result = lwm2m_send(lwm2mH, serverId, uris, uriCount, NULL, NULL);
        lwm2m_free(uris);
    } else {
        result = COAP_500_INTERNAL_SERVER_ERROR;
    }
    if (result != 0) {
        fprintf(stdout, "Send error: ");
        print_status(stdout, result);
        fprintf(stdout, "\r\n");
    }
    return;

syntax_error:
    if (uris != NULL) {
        lwm2m_free(uris);
    }
    fprintf(stdout, "Syntax error !\n");
}
#endif

static void prv_add(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_object_t *objectP;
    int res;

    /* unused parameter */
    (void)user_data;

    objectP = get_test_object();
    if (objectP == NULL) {
        fprintf(stdout, "Creating object 31024 failed.\r\n");
        return;
    }
    res = lwm2m_add_object(lwm2mH, objectP);
    if (res != 0) {
        fprintf(stdout, "Adding object 31024 failed: ");
        print_status(stdout, res);
        fprintf(stdout, "\r\n");
    } else {
        fprintf(stdout, "Object 31024 added.\r\n");
    }
    return;
}

static void prv_remove(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    int res;

    /* unused parameter */
    (void)user_data;

    res = lwm2m_remove_object(lwm2mH, 31024);
    if (res != 0) {
        fprintf(stdout, "Removing object 31024 failed: ");
        print_status(stdout, res);
        fprintf(stdout, "\r\n");
    } else {
        fprintf(stdout, "Object 31024 removed.\r\n");
    }
    return;
}

#ifdef LWM2M_BOOTSTRAP

static void prv_initiate_bootstrap(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_server_t *targetP;

    /* unused parameter */
    (void)user_data;

    // HACK !!!
    lwm2mH->state = STATE_BOOTSTRAP_REQUIRED;
    targetP = lwm2mH->bootstrapServerList;
    while (targetP != NULL) {
        targetP->lifetime = 0;
        targetP = targetP->next;
    }
}

static void prv_display_backup(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    int i;

    /* unused parameters */
    (void)lwm2mH;
    (void)buffer;
    (void)user_data;

    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        lwm2m_object_t *object = backupObjectArray[i];
        if (NULL != object) {
            switch (object->objID) {
            case LWM2M_SECURITY_OBJECT_ID:
                display_security_object(object);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                display_server_object(object);
                break;
            default:
                break;
            }
        }
    }
}

static void prv_backup_objects(lwm2m_context_t *context) {
    uint16_t i;

    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        if (NULL != backupObjectArray[i]) {
            switch (backupObjectArray[i]->objID) {
            case LWM2M_SECURITY_OBJECT_ID:
                clean_security_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                clean_server_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            default:
                break;
            }
        }
        backupObjectArray[i] = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));
        memset(backupObjectArray[i], 0, sizeof(lwm2m_object_t));
    }

    /*
     * Backup content of objects 0 (security) and 1 (server)
     */
    copy_security_object(backupObjectArray[0],
                         (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID));
    copy_server_object(backupObjectArray[1],
                       (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID));
}

static void prv_restore_objects(lwm2m_context_t *context) {
    lwm2m_object_t *targetP;

    /*
     * Restore content  of objects 0 (security) and 1 (server)
     */
    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SECURITY_OBJECT_ID);
    // first delete internal content
    clean_security_object(targetP);
    // then restore previous object
    copy_security_object(targetP, backupObjectArray[0]);

    targetP = (lwm2m_object_t *)LWM2M_LIST_FIND(context->objectList, LWM2M_SERVER_OBJECT_ID);
    // first delete internal content
    clean_server_object(targetP);
    // then restore previous object
    copy_server_object(targetP, backupObjectArray[1]);

    // restart the old servers
    fprintf(stdout, "[BOOTSTRAP] ObjectList restored\r\n");
}

static void update_bootstrap_info(lwm2m_client_state_t *previousBootstrapState, lwm2m_context_t *context) {
    if (*previousBootstrapState != context->state) {
        *previousBootstrapState = context->state;
        switch (context->state) {
        case STATE_BOOTSTRAPPING:
#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED
            fprintf(stdout, "[BOOTSTRAP] backup security and server objects\r\n");
#endif
            prv_backup_objects(context);
            break;
        default:
            break;
        }
    }
}

static void close_backup_object(void) {
    int i;
    for (i = 0; i < BACKUP_OBJECT_COUNT; i++) {
        if (NULL != backupObjectArray[i]) {
            switch (backupObjectArray[i]->objID) {
            case LWM2M_SECURITY_OBJECT_ID:
                clean_security_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                clean_server_object(backupObjectArray[i]);
                lwm2m_free(backupObjectArray[i]);
                break;
            default:
                break;
            }
        }
    }
}
#endif

static void prv_display_objects(lwm2m_context_t *lwm2mH, char *buffer, void *user_data) {
    lwm2m_object_t *object;

    /* unused parameter */
    (void)user_data;

    for (object = lwm2mH->objectList; object != NULL; object = object->next) {
        if (NULL != object) {
            switch (object->objID) {
            case LWM2M_SECURITY_OBJECT_ID:
                display_security_object(object);
                break;
            case LWM2M_SERVER_OBJECT_ID:
                display_server_object(object);
                break;
            case LWM2M_ACL_OBJECT_ID:
                break;
            case LWM2M_DEVICE_OBJECT_ID:
                display_device_object(object);
                break;
            case LWM2M_CONN_MONITOR_OBJECT_ID:
                break;
            case LWM2M_FIRMWARE_UPDATE_OBJECT_ID:
                // display_firmware_object(object); // Firmware object removed
                break;
            case LWM2M_LOCATION_OBJECT_ID:
                display_location_object(object);
                break;
            case LWM2M_CONN_STATS_OBJECT_ID:
                break;
            case TEST_OBJECT_ID:
                display_test_object(object);
                break;
            default:
                fprintf(stdout, "unknown object ID: %" PRIu16 "\n", object->objID);
                break;
            }
        }
    }
}

void print_usage(void) {
    fprintf(stdout, "Usage: lwm2mclient [OPTION]\r\n");
    fprintf(stdout, "Launch a LwM2M client.\r\n");
    fprintf(stdout, "Options:\r\n");
    fprintf(stdout, "  -n NAME\tSet the endpoint name of the Client. Default: testlwm2mclient\r\n");
    fprintf(stdout, "  -l PORT\tSet the local UDP port of the Client. Default: 56830\r\n");
    fprintf(stdout, "  -h HOST\tSet the hostname of the LwM2M Server to connect to. Default: localhost\r\n");
    fprintf(stdout,
            "  -p PORT\tSet the port of the LwM2M Server to connect to. Default: " LWM2M_STANDARD_PORT_STR "\r\n");
    fprintf(stdout, "  -4\t\tUse IPv4 connection. Default: IPv6 connection\r\n");
    fprintf(stdout, "  -t TIME\tSet the lifetime of the Client. Default: 300\r\n");
    fprintf(stdout, "  -b\t\tBootstrap requested.\r\n");
    fprintf(stdout, "  -S BYTES\tCoAP block size. Options: 16, 32, 64, 128, 256, 512, 1024. Default: %" PRIu16 "\r\n",
            (uint16_t)LWM2M_COAP_DEFAULT_BLOCK_SIZE);
#ifdef WITH_TINYDTLS
    fprintf(
        stdout,
        "  -i STRING\tSet the device management or bootstrap server PSK identity. If not set use none secure mode\r\n");
    fprintf(stdout, "  -s HEXSTRING\tSet the device management or bootstrap server Pre-Shared-Key. If not set use none "
                    "secure mode\r\n");
#endif
    fprintf(stdout, "  -f FILE\tSpecify path to SignalK settings.json file (enables SignalK WebSocket client)\r\n");
    fprintf(stdout, "\r\n");
}
// START wakatiwai

static char * server_get_uri(lwm2m_object_t * obj, uint16_t instanceId) {
    int size = 1;
    lwm2m_data_t * dataP = lwm2m_data_new(size);
    dataP->id = 0; // security server uri
    char * uriBuffer;

    obj->readFunc(NULL, instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
            (dataP->type == LWM2M_TYPE_STRING || dataP->type == LWM2M_TYPE_OPAQUE) &&
            dataP->value.asBuffer.length > 0) {
        uriBuffer = lwm2m_malloc(dataP->value.asBuffer.length + 1);
        memset(uriBuffer, 0, dataP->value.asBuffer.length + 1);
        strncpy(uriBuffer, (const char *) dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
        lwm2m_data_free(size, dataP);
        return uriBuffer;
    }
    lwm2m_data_free(size, dataP);
    return NULL;

}

static uint16_t object_id_contains(uint16_t objectId, uint16_t * objectIdArray, uint16_t len) {
    uint16_t result = 0;
    uint16_t i = 0;
    if (objectId < 1 || objectIdArray == NULL || len < 1) {
        return result;
    }
    for (; i < len; i++) {
        if (objectIdArray[i] == objectId) {
            result = 1;
            break;
        }
    }
    return result;
}

static uint16_t * parse_object_id_csv(const char * objectIdCsv, uint16_t * objCount) {
    uint16_t count = 1;
    uint16_t buffIdx = 0;
    uint16_t objectId = 0;
    uint16_t objectIndex = 0;
    uint16_t * objectIdArray;
    char buff[12];
    const char * c = objectIdCsv;
    for (; *c != '\0'; c++) {
        if (*c == ',') {
            ++count;
        }
    }
    c = objectIdCsv;
    objectIdArray = lwm2m_malloc(sizeof(uint16_t) * count);
    memset(objectIdArray, 0, sizeof(uint16_t) * count);
    for (; *c != '\0'; c++) {
        if (*c == ',') {
            if (buffIdx > 11) {
                fprintf(stderr, "Too long Object ID\r\n");
                lwm2m_free(objectIdArray);
                return NULL;
            }
            buff[buffIdx] = '\0';
            objectId = strtol(buff, NULL, 10);
            if (object_id_contains(objectId, objectIdArray, objectIndex)) {
                // duplicate object ID, ignored.
            } else  if (objectId > 3) {
                objectIdArray[objectIndex++] = objectId;
            } else {
                fprintf(stderr, "Invalid Object ID\r\n");
                lwm2m_free(objectIdArray);
                return NULL;
            }
            buffIdx = 0;
        } else {
            buff[buffIdx++] = *c;
        }
    }
    if (buffIdx > 11) {
        fprintf(stderr, "Too long Object ID\r\n");
        lwm2m_free(objectIdArray);
        return NULL;
    } else if (buffIdx > 0) {
        buff[buffIdx] = '\0';
        objectId = strtol(buff, NULL, 10);
        if (objectId > 3) {
            objectIdArray[objectIndex++] = objectId;
        } else {
            fprintf(stderr, "Invalid Object ID\r\n");
            lwm2m_free(objectIdArray);
            return NULL;
        }
    }
    *objCount = objectIndex;
#ifdef WITH_LOGS
    uint16_t i = 0;
    for (; i < objectIndex; i++) {
      fprintf(stderr, ">>> %hu => ObjectID:[%hu] \r\n", i, objectIdArray[i]);
    }
    // 4 objects(/0,/1,/2,/3) are implicitly included
    fprintf(stderr, ">>> %hu objects will be deployed as well as predfined 4 objects\r\n", *objCount);
#endif
    return objectIdArray;
}
// END wakatiwai

int main(int argc, char *argv[]) {
    client_data_t data;
    int result;
    lwm2m_context_t *lwm2mH = NULL;
    const char *localPort = "56830";
    const char *server = NULL;
    const char *serverPort = LWM2M_STANDARD_PORT_STR;
    const char *name = "testlwm2mclient";
    int lifetime = 300;
    time_t reboot_time = 0;
    int opt;
    bool bootstrapRequested = false;
    bool serverPortChanged = false;

    // SignalK WebSocket parameters
    bool signalk_enabled = false;
    bool signalk_started = false;
    const char *settings_file = NULL;

#ifdef LWM2M_BOOTSTRAP
    lwm2m_client_state_t previousState = STATE_INITIAL;
#endif

    char *pskId = NULL;
#ifdef WITH_TINYDTLS
    char *psk = NULL;
#endif
    uint16_t pskLen = -1;
    char *pskBuffer = NULL;

    /*
     * The function start by setting up the command line interface (which may or not be useful depending on your
     * project)
     *
     * This is an array of commands describes as { name, description, long description, callback, userdata }.
     * The firsts tree are easy to understand, the callback is the function that will be called when this command is
     * typed and in the last one will be stored the lwm2m context (allowing access to the server settings and the
     * objects).
     */
    command_desc_t commands[] = {
        {"list", "List known servers.", NULL, prv_output_servers, NULL},
        {"change", "Change the value of resource.",
         " change URI [DATA]\r\n"
         "   URI: uri of the resource such as /3/0, /3/0/2\r\n"
         "   DATA: (optional) new value\r\n",
         prv_change, NULL},
        {"update", "Trigger a registration update",
         " update SERVER\r\n"
         "   SERVER: short server id such as 123\r\n",
         prv_update, NULL},
#ifndef LWM2M_VERSION_1_0
        {"send", "Send one or more resources",
         " send SERVER URI [URI...]\r\n"
         "   SERVER: short server id such as 123. 0 for all.\r\n"
         "   URI: uri of the resource such as /3/0, /3/0/2\r\n",
         prv_send, NULL},
#endif
#ifdef LWM2M_BOOTSTRAP
        {"bootstrap", "Initiate a DI bootstrap process", NULL, prv_initiate_bootstrap, NULL},
        {"dispb",
         "Display current backup of objects/instances/resources\r\n"
         "\t(only security and server objects are backupped)",
         NULL, prv_display_backup, NULL},
#endif
        {"ls", "List Objects and Instances", NULL, prv_object_list, NULL},
        {"disp", "Display current objects/instances/resources", NULL, prv_display_objects, NULL},
        {"dump", "Dump an Object",
         "dump URI"
         "URI: uri of the Object or Instance such as /3/0, /1\r\n",
         prv_object_dump, NULL},
        {"add", "Add support of object 31024", NULL, prv_add, NULL},
        {"rm", "Remove support of object 31024", NULL, prv_remove, NULL},
        {"quit", "Quit the client gracefully.", NULL, prv_quit, NULL},
        {"^C", "Quit the client abruptly (without sending a de-register message).", NULL, NULL, NULL},

        COMMAND_END_LIST};

    memset(&data, 0, sizeof(client_data_t));
    data.addressFamily = AF_INET6;

    opt = 1;
    while (opt < argc) {
        if (argv[opt] == NULL || argv[opt][0] != '-' || argv[opt][2] != 0) {
            print_usage();
            return 0;
        }
        switch (argv[opt][1]) {
        case 'b':
            bootstrapRequested = true;
            if (!serverPortChanged)
                serverPort = LWM2M_BSSERVER_PORT_STR;
            break;
        case 't':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            if (1 != sscanf(argv[opt], "%d", &lifetime)) {
                print_usage();
                return 0;
            }
            break;
#ifdef WITH_TINYDTLS
        case 'i':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            pskId = argv[opt];
            break;
        case 's':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            psk = argv[opt];
            break;
#endif
        case 'n':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            name = argv[opt];
            break;
        case 'l':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            localPort = argv[opt];
            break;
        case 'h':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            server = argv[opt];
            break;
        case 'p':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            serverPort = argv[opt];
            serverPortChanged = true;
            break;
        case '4':
            data.addressFamily = AF_INET;
            break;
        case 'S':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            uint16_t coap_block_size_arg;
            if (1 == sscanf(argv[opt], "%" SCNu16, &coap_block_size_arg) &&
                lwm2m_set_coap_block_size(coap_block_size_arg)) {
                break;
            } else {
                print_usage();
                return 0;
            }
        case 'f':
            opt++;
            if (opt >= argc) {
                print_usage();
                return 0;
            }
            settings_file = argv[opt];
            signalk_enabled = true;  // Enable SignalK when settings file is provided
            break;
        default:
            print_usage();
            return 0;
        }
        opt += 1;
    }

    if (!server) {
        server = (AF_INET == data.addressFamily ? DEFAULT_SERVER_IPV4 : DEFAULT_SERVER_IPV6);
    }

    /*
     *This call an internal function that create an IPV6 socket on the port 5683.
     */
    fprintf(stderr, "Trying to bind LwM2M Client to port %s\r\n", localPort);
    data.sock = lwm2m_create_socket(localPort, data.addressFamily);
    if (data.sock < 0) {
        fprintf(stderr, "Failed to open socket: %d %s\r\n", errno, strerror(errno));
        return -1;
    }

    /*
     * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
     * Those functions are located in their respective object file.
     */
#ifdef WITH_TINYDTLS
    if (psk != NULL) {
        pskLen = strlen(psk) / 2; // NOSONAR
        pskBuffer = malloc(pskLen);

        if (NULL == pskBuffer) {
            fprintf(stderr, "Failed to create PSK binary buffer\r\n");
            return -1;
        }
        // Hex string to binary
        char *h = psk;
        char *b = pskBuffer;
        char xlate[] = "0123456789ABCDEF";

        for (; *h; h += 2, ++b) {
            char *l = strchr(xlate, toupper(*h));
            char *r = strchr(xlate, toupper(*(h + 1)));

            if (!r || !l) {
                fprintf(stderr, "Failed to parse Pre-Shared-Key HEXSTRING\r\n");
                return -1;
            }

            *b = ((l - xlate) << 4) + (r - xlate);
        }
    }
#endif

    char serverUri[50];
    int serverId = 123;
#ifdef WITH_TINYDTLS
    sprintf(serverUri, "coaps://%s:%s", server, serverPort); // NOSONAR
#else
    sprintf(serverUri, "coap://%s:%s", server, serverPort); // NOSONAR
#endif
#ifdef LWM2M_BOOTSTRAP
    objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, bootstrapRequested);
#else
    objArray[0] = get_security_object(serverId, serverUri, pskId, pskBuffer, pskLen, false);
#endif
    if (NULL == objArray[0]) {
        fprintf(stderr, "Failed to create security object\r\n");
        return -1;
    }
    data.securityObjP = objArray[0];

    objArray[1] = get_server_object(serverId, "U", lifetime, false);
    if (NULL == objArray[1]) {
        fprintf(stderr, "Failed to create server object\r\n");
        return -1;
    }

    objArray[2] = get_object_device();
    if (NULL == objArray[2]) {
        fprintf(stderr, "Failed to create Device object\r\n");
        return -1;
    }

    objArray[3] = get_object_firmware();
    if (NULL == objArray[3]) {
        fprintf(stderr, "Failed to create Firmware object\r\n");
        return -1;
    }

    objArray[4] = get_object_location();
    if (NULL == objArray[4]) {
        fprintf(stderr, "Failed to create location object\r\n");
        return -1;
    }

    objArray[5] = get_object_conn_m();
    if (NULL == objArray[5]) {
        fprintf(stderr, "Failed to create connectivity monitoring object\r\n");
        return -1;
    }

    // Add Generic Sensor Object (3300) for environmental data
    objArray[6] = get_object_generic_sensor("environment.temperature", "C");
    if (NULL == objArray[6]) {
        fprintf(stderr, "Failed to create generic sensor object\r\n");
        return -1;
    }

    // Add Power Measurement Object (3305) for electrical monitoring
    objArray[7] = get_power_measurement_object();
    if (NULL == objArray[7]) {
        fprintf(stderr, "Failed to create power measurement object\r\n");
        return -1;
    }

    // Add Energy Object (3331) for cumulative energy tracking
    objArray[8] = get_energy_object();
    if (NULL == objArray[8]) {
        fprintf(stderr, "Failed to create energy object\r\n");
        return -1;
    }

    // Add Actuation Object (3306) for switch/relay control
    objArray[9] = get_actuation_object();
    if (NULL == objArray[9]) {
        fprintf(stderr, "Failed to create actuation object\r\n");
        return -1;
    }

    // Connectivity statistics object removed due to crashes

    // Note: Access control object removed to keep OBJ_COUNT at 8
    /*
    int instId = 0;
    objArray[7] = acc_ctrl_create_object();
    if (NULL == objArray[7]) {
        fprintf(stderr, "Failed to create Access Control object\r\n");
        return -1;
    } else if (acc_ctrl_obj_add_inst(objArray[7], instId, 3, 0, serverId) == false) {
        fprintf(stderr, "Failed to create Access Control object instance\r\n");
        return -1;
    } else if (acc_ctrl_oi_add_ac_val(objArray[7], instId, 0, 0xF) == false) {
        fprintf(stderr, "Failed to create Access Control ACL default resource\r\n");
        return -1;
    } else if (acc_ctrl_oi_add_ac_val(objArray[7], instId, 999, 0x1) == false) {
        fprintf(stderr, "Failed to create Access Control ACL resource for serverId: 999\r\n");
        return -1;
    }
    */
    /*
     * The liblwm2m library is now initialized with the functions that will be in
     * charge of communication
     */
    lwm2mH = lwm2m_init(&data);
    if (NULL == lwm2mH) {
        fprintf(stderr, "lwm2m_init() failed\r\n");
        return -1;
    }
#ifdef WITH_TINYDTLS
    data.lwm2mH = lwm2mH;
#endif

    /*
     * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
     * the number of objects we will be passing through and the objects array
     */
    result = lwm2m_configure(lwm2mH, name, NULL, NULL, OBJ_COUNT, objArray);
    if (result != 0) {
        fprintf(stderr, "lwm2m_configure() failed: 0x%X\r\n", result);
        return -1;
    }

    signal(SIGINT, handle_sigint);

    /**
     * Initialize value changed callback.
     */
    init_value_change(lwm2mH);

    // Initialize SignalK bridge system
    bridge_init();

    fprintf(stdout, "LWM2M Client \"%s\" started on port %s\r\n", name, localPort);
    if (signalk_enabled) {
        fprintf(stdout, "SignalK integration enabled - will start after LwM2M registration\r\n");
    }
    fprintf(stdout, "> ");
    fflush(stdout);
    /*
     * We now enter in a while loop that will handle the communications from the server
     */
    while (0 == g_quit) {
        struct timeval tv;
        fd_set readfds;

        if (g_reboot) {
            time_t tv_sec;

            tv_sec = lwm2m_gettime();

            if (0 == reboot_time) {
                reboot_time = tv_sec + 5;
            }
            if (reboot_time < tv_sec) {
                /*
                 * Message should normally be lost with reboot ...
                 */
                fprintf(stderr, "reboot time expired, rebooting ...");
                system_reboot();
            } else {
                tv.tv_sec = reboot_time - tv_sec;
            }
        } else {
            tv.tv_sec = 60;
        }
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(data.sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        /*
         * This function does two things:
         *  - first it does the work needed by liblwm2m (eg. (re)sending some packets).
         *  - Secondly it adjusts the timeout value (default 60s) depending on the state of the transaction
         *    (eg. retransmission) and the time between the next operation
         */
        result = lwm2m_step(lwm2mH, &(tv.tv_sec));
        
        // Prevent busy loops by ensuring minimum timeout
        if (tv.tv_sec == 0 && tv.tv_usec == 0) {
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms minimum timeout
        }
        
        // Only print state changes, not every loop iteration
        static int last_state = -1;
        if (lwm2mH->state != last_state) {
            fprintf(stdout, " -> State: ");
            switch (lwm2mH->state) {
            case STATE_INITIAL:
                fprintf(stdout, "STATE_INITIAL\r\n");
                break;
            case STATE_BOOTSTRAP_REQUIRED:
                fprintf(stdout, "STATE_BOOTSTRAP_REQUIRED\r\n");
                break;
            case STATE_BOOTSTRAPPING:
                fprintf(stdout, "STATE_BOOTSTRAPPING\r\n");
                break;
            case STATE_REGISTER_REQUIRED:
                fprintf(stdout, "STATE_REGISTER_REQUIRED\r\n");
                break;
            case STATE_REGISTERING:
                fprintf(stdout, "STATE_REGISTERING\r\n");
                break;
            case STATE_READY:
                fprintf(stdout, "STATE_READY\r\n");
                break;
            default:
                fprintf(stdout, "Unknown...\r\n");
                break;
            }
            last_state = lwm2mH->state;
        }
        
        // Check for quit signal early for faster response
        if (g_quit != 0) {
            fprintf(stdout, "Shutting down...\r\n");
            break;
        }
        
        // Start SignalK WebSocket client when LwM2M registration is complete
        if (signalk_enabled && !signalk_started && lwm2mH->state == STATE_READY) {
            fprintf(stdout, "[SIGNALK] LwM2M registration complete - starting SignalK WebSocket client\r\n");
            
            if (signalk_ws_start(NULL, 0, settings_file) != 0) {
                fprintf(stderr, "[SIGNALK] Warning: Failed to start SignalK WebSocket client (server may not be running)\r\n");
                fprintf(stderr, "[SIGNALK] Continuing without SignalK integration...\r\n");
            } else {
                fprintf(stdout, "[SIGNALK] WebSocket client started successfully\r\n");
                fprintf(stdout, "[SIGNALK] Bridge system ready - marine data will be bridged to LwM2M objects\r\n");
                signalk_started = true;
            }
        }
        
        if (result != 0) {
            fprintf(stderr, "lwm2m_step() failed: 0x%X\r\n", result);
#ifdef LWM2M_BOOTSTRAP
            if (previousState == STATE_BOOTSTRAPPING) {
#if LWM2M_LOG_LEVEL != LWM2M_LOG_DISABLED
                fprintf(stdout, "[BOOTSTRAP] restore security and server objects\r\n");
#endif
                prv_restore_objects(lwm2mH);
                lwm2mH->state = STATE_INITIAL;
            } else
#endif
                return -1;
        }
#ifdef LWM2M_BOOTSTRAP
        update_bootstrap_info(&previousState, lwm2mH);
#endif
        /*
         * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
         * with the precedent function)
         */
        result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

        if (result < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "Error in select(): %d %s\r\n", errno, strerror(errno));
            }
        } else if (result > 0) {
            uint8_t buffer[MAX_PACKET_SIZE];
            ssize_t numBytes;

            /*
             * If an event happens on the socket
             */
            if (FD_ISSET(data.sock, &readfds)) {
                struct sockaddr_storage addr;
                socklen_t addrLen;

                addrLen = sizeof(addr);

                /*
                 * We retrieve the data received
                 */
                numBytes = recvfrom(data.sock, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (0 > numBytes) {
                    fprintf(stderr, "Error in recvfrom(): %d %s\r\n", errno, strerror(errno));
                } else if (numBytes >= MAX_PACKET_SIZE) {
                    fprintf(stderr, "Received packet >= MAX_PACKET_SIZE\r\n");
                } else if (0 < numBytes) {
                    char s[INET6_ADDRSTRLEN];
                    in_port_t port;

#ifdef WITH_TINYDTLS
                    lwm2m_dtls_connection_t *connP;
#else
                    lwm2m_connection_t *connP;
#endif
                    if (AF_INET == addr.ss_family) {
                        struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
                        port = saddr->sin_port;
                    } else if (AF_INET6 == addr.ss_family) {
                        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&addr;
                        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
                        port = saddr->sin6_port;
                    }
                    fprintf(stderr, "%zd bytes received from [%s]:%hu\r\n", numBytes, s, ntohs(port));

                    /*
                     * Display it in the STDERR
                     */
                    output_buffer(stderr, buffer, (size_t)numBytes, 0);

                    connP = lwm2m_connection_find(data.connList, &addr, addrLen);
                    if (connP != NULL) {
                        /*
                         * Let liblwm2m respond to the query depending on the context
                         */
#ifdef WITH_TINYDTLS
                        result = lwm2m_connection_handle_packet(connP, buffer, numBytes);
                        if (0 != result) {
                            printf("error handling message %d\n", result);
                        }
#else
                        lwm2m_handle_packet(lwm2mH, buffer, (size_t)numBytes, connP);
#endif
                        // conn_s_updateRxStatistic removed - no connectivity statistics object
                    } else {
                        fprintf(stderr, "received bytes ignored!\r\n");
                    }
                }
            }

            /*
             * If the event happened on the SDTIN
             */
            else if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char *line = NULL;
                size_t bufLen = 0;

                numBytes = getline(&line, &bufLen, stdin);

                if (numBytes > 1) {
                    line[numBytes] = 0;
                    /*
                     * We call the corresponding callback of the typed command passing it the buffer for further
                     * arguments
                     */
                    handle_command(lwm2mH, commands, line);
                }
                if (g_quit == 0) {
                    fprintf(stdout, "\r\n> ");
                    fflush(stdout);
                } else {
                    fprintf(stdout, "\r\n");
                }

                if (line) {
                    lwm2m_free(line);
                }
            }
        }
        
        // Check for quit signal again before next iteration
        if (g_quit != 0) {
            break;
        }
    }

    /*
     * Finally when the loop is left smoothly - asked by user in the command line interface - we unregister our client
     * from it
     */
    
    // Cleanup SignalK WebSocket client first to avoid thread issues
    if (signalk_enabled && signalk_started) {
        fprintf(stdout, "Stopping SignalK WebSocket client\r\n");
        signalk_ws_stop();
    }
    
    // Handle both graceful quit (1) and signal quit (2)
    if (g_quit == 1 || g_quit == 2) {
#ifdef WITH_TINYDTLS
        if (pskBuffer) {
            free(pskBuffer);
        }
#endif

#ifdef LWM2M_BOOTSTRAP
        close_backup_object();
#endif
        // Close connections first before lwm2m_close
        close(data.sock);
        lwm2m_connection_free(data.connList);
        
        // Only call lwm2m_close if we have a valid context
        if (lwm2mH) {
            lwm2m_close(lwm2mH);
        }
    } else {
        close(data.sock);
        lwm2m_connection_free(data.connList);
    }

    // Clean up objects with null checks
    if (objArray[0]) {
        clean_security_object(objArray[0]);
        lwm2m_free(objArray[0]);
    }
    if (objArray[1]) {
        clean_server_object(objArray[1]);
        lwm2m_free(objArray[1]);
    }
    if (objArray[2]) {
        free_object_device(objArray[2]);
    }
    if (objArray[3]) {
        free_object_firmware(objArray[3]);
    }
    if (objArray[4]) {
        free_object_location(objArray[4]);
    }
    if (objArray[5]) {
        free_object_conn_m(objArray[5]);
    }
    if (objArray[6]) {
        free_object_generic_sensor(objArray[6]);
    }
    if (objArray[7]) {
        free_power_measurement_object(objArray[7]);
    }
    if (objArray[8]) {
        free_energy_object(objArray[8]);
    }
    if (objArray[9]) {
        free_actuation_object(objArray[9]);
    }
    // free_object_conn_s - removed with connectivity statistics object

    return 0;
}
