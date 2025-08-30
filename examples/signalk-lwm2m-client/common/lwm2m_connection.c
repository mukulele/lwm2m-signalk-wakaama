#include "lwm2m_connection.h"
#include "lwm2mclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

// External globals from lwm2mclient.c
extern int g_quit;
extern int g_reboot;
extern command_desc_t commands[];

// Internal state structure
struct lwm2m_connection_state_t {
    lwm2m_context_t *context;
    client_data_t client_data;
    lwm2m_connection_config_t config;
    bool is_running;
    time_t reboot_time;
    lwm2m_object_t *objects[16];  // Array to hold LwM2M objects
    int object_count;
};

/**
 * Initialize LwM2M objects for the client
 */
static int lwm2m_connection_init_objects(lwm2m_connection_state_t *state) {
    char serverUri[50];
    int serverId = 123;
    
    // Clear objects array
    memset(state->objects, 0, sizeof(state->objects));
    state->object_count = 0;
    
#ifdef WITH_TINYDTLS
    sprintf(serverUri, "coaps://%s:%s", state->config.server_host, state->config.server_port);
#else
    sprintf(serverUri, "coap://%s:%s", state->config.server_host, state->config.server_port);
#endif

    // Security Object
#ifdef LWM2M_BOOTSTRAP
    state->objects[0] = get_security_object(serverId, serverUri, state->config.psk_id, 
                                          state->config.psk, state->config.psk_len, 
                                          state->config.bootstrap_requested);
#else
    state->objects[0] = get_security_object(serverId, serverUri, state->config.psk_id, 
                                          state->config.psk, state->config.psk_len, false);
#endif
    if (!state->objects[0]) {
        fprintf(stderr, "[LwM2M] Failed to create security object\n");
        return -1;
    }
    state->client_data.securityObjP = state->objects[0];
    state->object_count++;

    // Server Object
    state->objects[1] = get_server_object(serverId, "U", state->config.lifetime, false);
    if (!state->objects[1]) {
        fprintf(stderr, "[LwM2M] Failed to create server object\n");
        return -1;
    }
    state->client_data.serverObject = state->objects[1];
    state->object_count++;

    // Device Object
    state->objects[2] = get_object_device();
    if (!state->objects[2]) {
        fprintf(stderr, "[LwM2M] Failed to create device object\n");
        return -1;
    }
    state->object_count++;

    // Firmware Object
    state->objects[3] = get_object_firmware();
    if (!state->objects[3]) {
        fprintf(stderr, "[LwM2M] Failed to create firmware object\n");
        return -1;
    }
    state->object_count++;

    // Location Object
    state->objects[4] = get_object_location();
    if (!state->objects[4]) {
        fprintf(stderr, "[LwM2M] Failed to create location object\n");
        return -1;
    }
    state->object_count++;

    // Add marine IoT objects
    // Temperature sensor
    state->objects[5] = get_object_generic_sensor(3300, 0, "temperature", "°C", 0.0, 100.0, 20.5);
    if (state->objects[5]) state->object_count++;

    // Power measurement objects
    state->objects[6] = get_object_power_measurement();
    if (state->objects[6]) state->object_count++;

    // Energy objects
    state->objects[7] = get_object_energy(); 
    if (state->objects[7]) state->object_count++;

    // Actuation objects
    state->objects[8] = get_object_actuation();
    if (state->objects[8]) state->object_count++;

    fprintf(stdout, "[LwM2M] Initialized %d objects for marine IoT\n", state->object_count);
    return 0;
}

/**
 * Initialize socket for LwM2M communication
 */
static int lwm2m_connection_init_socket(lwm2m_connection_state_t *state) {
    char *localPort = (char*)state->config.local_port;
    
    state->client_data.sock = create_socket(localPort, state->config.address_family);
    if (state->client_data.sock < 0) {
        fprintf(stderr, "[LwM2M] Failed to open socket: %d %s\n", errno, strerror(errno));
        return -1;
    }
    
    state->client_data.addressFamily = state->config.address_family;
    state->client_data.connList = NULL;
    
    return 0;
}

lwm2m_connection_state_t* lwm2m_connection_init(const lwm2m_connection_config_t *config) {
    if (!config) {
        fprintf(stderr, "[LwM2M] Invalid configuration\n");
        return NULL;
    }
    
    lwm2m_connection_state_t *state = calloc(1, sizeof(lwm2m_connection_state_t));
    if (!state) {
        fprintf(stderr, "[LwM2M] Failed to allocate connection state\n");
        return NULL;
    }
    
    // Copy configuration
    memcpy(&state->config, config, sizeof(lwm2m_connection_config_t));
    state->is_running = false;
    state->reboot_time = 0;
    
    // Initialize socket
    if (lwm2m_connection_init_socket(state) != 0) {
        free(state);
        return NULL;
    }
    
    // Initialize objects
    if (lwm2m_connection_init_objects(state) != 0) {
        close(state->client_data.sock);
        free(state);
        return NULL;
    }
    
    fprintf(stdout, "[LwM2M] Connection module initialized\n");
    return state;
}

int lwm2m_connection_start(lwm2m_connection_state_t *state) {
    if (!state) return -1;
    
    // Create LwM2M context
    state->context = lwm2m_init(&state->client_data);
    if (!state->context) {
        fprintf(stderr, "[LwM2M] Failed to initialize context\n");
        return -1;
    }
    
#ifdef WITH_TINYDTLS
    state->client_data.lwm2mH = state->context;
#endif
    
    // Configure objects
    int result = lwm2m_configure(state->context, state->config.endpoint_name, NULL, NULL, 
                                state->object_count, state->objects);
    if (result != 0) {
        fprintf(stderr, "[LwM2M] Failed to configure client: 0x%X\n", result);
        lwm2m_close(state->context);
        return -1;
    }
    
    // Initialize value changed callback
    init_value_change(state->context);
    
    state->is_running = true;
    
    fprintf(stdout, "[LwM2M] Client \"%s\" started on port %s\n", 
            state->config.endpoint_name, state->config.local_port);
    
    return 0;
}

int lwm2m_connection_process(lwm2m_connection_state_t *state, int timeout_ms) {
    if (!state || !state->is_running || !state->context) {
        return -1;
    }
    
    struct timeval tv;
    fd_set readfds;
    int result;
    
    // Handle reboot logic
    if (lwm2m_connection_handle_reboot(state)) {
        return -1; // Reboot imminent
    }
    
    // Set timeout
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Setup file descriptors
    FD_ZERO(&readfds);
    FD_SET(state->client_data.sock, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    // Process LwM2M state machine
    result = lwm2m_step(state->context, &(tv.tv_sec));
    
    // Prevent busy loops
    if (tv.tv_sec == 0 && tv.tv_usec == 0) {
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms minimum timeout
    }
    
    // Wait for events
    result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
    
    if (result < 0) {
        if (errno != EINTR) {
            fprintf(stderr, "[LwM2M] Error in select(): %d %s\n", errno, strerror(errno));
            return -1;
        }
    } else if (result > 0) {
        uint8_t buffer[1024];
        ssize_t numBytes;
        
        // Handle socket data
        if (FD_ISSET(state->client_data.sock, &readfds)) {
            struct sockaddr_storage addr;
            socklen_t addrLen = sizeof(addr);
            
            numBytes = recvfrom(state->client_data.sock, buffer, sizeof(buffer), 0, 
                              (struct sockaddr *)&addr, &addrLen);
            
            if (numBytes < 0) {
                fprintf(stderr, "[LwM2M] Error receiving packet: %s\n", strerror(errno));
                return -1;
            }
            
            if (numBytes > 0) {
                lwm2m_connection_t *connP = lwm2m_connection_find(state->client_data.connList, &addr, addrLen);
                if (connP) {
#ifdef WITH_TINYDTLS
                    result = lwm2m_connection_handle_packet(connP, buffer, numBytes);
                    if (result != 0) {
                        fprintf(stderr, "[LwM2M] Error handling message: %d\n", result);
                    }
#else
                    lwm2m_handle_packet(state->context, buffer, (size_t)numBytes, connP);
#endif
                } else {
                    fprintf(stderr, "[LwM2M] Received bytes from unknown source\n");
                }
            }
        }
        
        // Handle user input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char *line = NULL;
            size_t bufLen = 0;
            
            numBytes = getline(&line, &bufLen, stdin);
            if (numBytes > 1) {
                line[numBytes] = 0;
                lwm2m_connection_handle_command(state, line);
            }
            if (line) free(line);
        }
    }
    
    return g_quit;
}

bool lwm2m_connection_is_ready(const lwm2m_connection_state_t *state) {
    return (state && state->context && state->context->state == STATE_READY);
}

lwm2m_client_state_t lwm2m_connection_get_state(const lwm2m_connection_state_t *state) {
    if (!state || !state->context) {
        return STATE_INITIAL;
    }
    return state->context->state;
}

int lwm2m_connection_handle_command(lwm2m_connection_state_t *state, const char *command) {
    if (!state || !command) return -1;
    
    handle_command(state->context, commands, command);
    return 0;
}

bool lwm2m_connection_handle_reboot(lwm2m_connection_state_t *state) {
    if (!state || !g_reboot) return false;
    
    time_t tv_sec = lwm2m_gettime();
    
    if (state->reboot_time == 0) {
        state->reboot_time = tv_sec + 5;
    }
    
    if (state->reboot_time < tv_sec) {
        fprintf(stderr, "[LwM2M] Reboot time expired, rebooting...\n");
        system_reboot();
        return true;
    }
    
    return false;
}

int lwm2m_connection_get_socket(const lwm2m_connection_state_t *state) {
    return (state ? state->client_data.sock : -1);
}

void lwm2m_connection_cleanup(lwm2m_connection_state_t *state) {
    if (!state) return;
    
    state->is_running = false;
    
    // Close socket
    if (state->client_data.sock >= 0) {
        close(state->client_data.sock);
    }
    
    // Free connections
    if (state->client_data.connList) {
        lwm2m_connection_free(state->client_data.connList);
    }
    
    // Close LwM2M context
    if (state->context) {
        lwm2m_close(state->context);
    }
    
    // Note: Objects are cleaned up by lwm2m_close()
    
    fprintf(stdout, "[LwM2M] Connection module cleaned up\n");
    free(state);
}
