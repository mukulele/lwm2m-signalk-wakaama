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
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
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

*/

/*
 * This object is single instance only, and is mandatory to all LwM2M device as it describe the object such as its
 * manufacturer, model, etc...
 */

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <errno.h>

#define PRV_MANUFACTURER "Open Mobile Alliance"
#define PRV_MODEL_NUMBER "Lightweight M2M Client"
#define PRV_SERIAL_NUMBER "345000123"
#define PRV_FIRMWARE_VERSION "1.0"
#define PRV_POWER_SOURCE_1 1
#define PRV_POWER_SOURCE_2 5
#define PRV_POWER_VOLTAGE_1 3800
#define PRV_POWER_VOLTAGE_2 5000
#define PRV_POWER_CURRENT_1 125
#define PRV_POWER_CURRENT_2 900
#define PRV_BATTERY_LEVEL 100
#define PRV_MEMORY_FREE 15
#define PRV_ERROR_CODE 0
#define PRV_TIME_ZONE "Europe/Berlin"
#define PRV_BINDING_MODE "U"

#define PRV_OFFSET_MAXLEN 7 //+HH:MM\0 at max
#define PRV_TLV_BUFFER_SIZE 128

// Resource Id's:
#define RES_O_MANUFACTURER 0
#define RES_O_MODEL_NUMBER 1
#define RES_O_SERIAL_NUMBER 2
#define RES_O_FIRMWARE_VERSION 3
#define RES_M_REBOOT 4
#define RES_O_FACTORY_RESET 5
#define RES_O_AVL_POWER_SOURCES 6
#define RES_O_POWER_SOURCE_VOLTAGE 7
#define RES_O_POWER_SOURCE_CURRENT 8
#define RES_O_BATTERY_LEVEL 9
#define RES_O_MEMORY_FREE 10
#define RES_M_ERROR_CODE 11
#define RES_O_RESET_ERROR_CODE 12
#define RES_O_CURRENT_TIME 13
#define RES_O_UTC_OFFSET 14
#define RES_O_TIMEZONE 15
#define RES_M_BINDING_MODES 16
// since TS 20141126-C:
#define RES_O_DEVICE_TYPE 17
#define RES_O_HARDWARE_VERSION 18
#define RES_O_SOFTWARE_VERSION 19
#define RES_O_BATTERY_STATUS 20
#define RES_O_MEMORY_TOTAL 21

typedef struct {
    int64_t free_memory;
    int64_t error;
    int64_t time;
    uint8_t battery_level;
    char time_offset[PRV_OFFSET_MAXLEN];
} device_data_t;

// Hardware detection globals
static char g_manufacturer[64] = {0};
static char g_model[64] = {0};
static char g_serial[32] = {0};
static char g_os_version[128] = {0};
static char g_kernel_version[64] = {0};
static bool g_hardware_detected = false;

/**
 * Read a single line from a file
 */
static bool prv_read_file_line(const char *filename, char *buffer, size_t buffer_size) {
    FILE *file = fopen(filename, "r");
    if (!file) return false;
    
    bool success = (fgets(buffer, buffer_size, file) != NULL);
    fclose(file);
    
    if (success) {
        // Remove trailing newline
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
    }
    
    return success;
}

/**
 * Extract field value from /proc/cpuinfo
 */
static bool prv_get_cpuinfo_field(const char *field_name, char *buffer, size_t buffer_size) {
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (!file) return false;
    
    char line[256];
    size_t field_len = strlen(field_name);
    bool found = false;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, field_name, field_len) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                colon++; // Skip colon
                while (*colon == ' ' || *colon == '\t') colon++; // Skip whitespace
                
                strncpy(buffer, colon, buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                
                // Remove trailing newline
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len-1] == '\n') {
                    buffer[len-1] = '\0';
                }
                
                found = true;
                break;
            }
        }
    }
    
    fclose(file);
    return found;
}

/**
 * Detect operating system information
 */
static bool prv_detect_os_info(void) {
    struct utsname uts;
    if (uname(&uts) == 0) {
        // Get kernel version
        strncpy(g_kernel_version, uts.release, sizeof(g_kernel_version) - 1);
        
        // Try to get OS version from /etc/os-release
        FILE *os_file = fopen("/etc/os-release", "r");
        if (os_file) {
            char line[256];
            while (fgets(line, sizeof(line), os_file)) {
                if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                    char *start = strchr(line, '"');
                    if (start) {
                        start++;
                        char *end = strchr(start, '"');
                        if (end) {
                            *end = '\0';
                            strncpy(g_os_version, start, sizeof(g_os_version) - 1);
                        }
                    }
                    break;
                }
            }
            fclose(os_file);
        }
        
        // Fallback if no PRETTY_NAME found
        if (strlen(g_os_version) == 0) {
            snprintf(g_os_version, sizeof(g_os_version), "%s %s", uts.sysname, uts.release);
        }
        
        return true;
    }
    
    // Fallback
    strncpy(g_os_version, "Linux (Unknown)", sizeof(g_os_version) - 1);
    strncpy(g_kernel_version, "Unknown", sizeof(g_kernel_version) - 1);
    return false;
}

/**
 * Detect Raspberry Pi model from device tree
 */
static bool prv_detect_hardware(void) {
    if (g_hardware_detected) {
        return true;
    }

    // Detect OS information first
    prv_detect_os_info();

    // Set manufacturer (always Raspberry Pi Foundation for Pi devices)
    strncpy(g_manufacturer, "Raspberry Pi Foundation", sizeof(g_manufacturer) - 1);
    
    // Try device tree model first
    if (!prv_read_file_line("/proc/device-tree/model", g_model, sizeof(g_model))) {
        // Fallback to cpuinfo Hardware field
        if (!prv_get_cpuinfo_field("Hardware", g_model, sizeof(g_model))) {
            // Last resort - try to detect from revision
            char revision[32];
            if (prv_get_cpuinfo_field("Revision", revision, sizeof(revision))) {
                // Basic Pi model detection from revision codes
                if (strstr(revision, "a02082") || strstr(revision, "a22082")) {
                    strncpy(g_model, "Raspberry Pi 3 Model B", sizeof(g_model) - 1);
                } else if (strstr(revision, "a020d3")) {
                    strncpy(g_model, "Raspberry Pi 3 Model B+", sizeof(g_model) - 1);
                } else if (strstr(revision, "a03111") || strstr(revision, "b03111") || strstr(revision, "c03111")) {
                    strncpy(g_model, "Raspberry Pi 4 Model B", sizeof(g_model) - 1);
                } else {
                    snprintf(g_model, sizeof(g_model), "Raspberry Pi (Rev: %s)", revision);
                }
            } else {
                strncpy(g_model, "SignalK-LwM2M Marine Gateway", sizeof(g_model) - 1);
            }
        }
    }
    
    // Get serial number
    if (!prv_get_cpuinfo_field("Serial", g_serial, sizeof(g_serial))) {
        strncpy(g_serial, "SIGNALK-MARINE-001", sizeof(g_serial) - 1);
    }
    
    g_hardware_detected = true;
    printf("[Device] Hardware detected: %s %s (Serial: %s)\n", g_manufacturer, g_model, g_serial);
    printf("[Device] OS: %s (Kernel: %s)\n", g_os_version, g_kernel_version);
    
    return true;
}

/**
 * Get current free memory in MB
 */
static uint32_t prv_get_free_memory_mb(void) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        uint64_t free_kb = (uint64_t)(si.freeram * si.mem_unit) / 1024;
        return (uint32_t)(free_kb / 1024);
    }
    return PRV_MEMORY_FREE; // Fallback to default
}

/**
 * Get disk usage information
 */
static bool prv_get_disk_usage(uint64_t *used_mb, uint64_t *total_mb, float *usage_percent) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0) {
        uint64_t total_kb = (uint64_t)(vfs.f_blocks * vfs.f_frsize) / 1024;
        uint64_t free_kb = (uint64_t)(vfs.f_bavail * vfs.f_frsize) / 1024;
        uint64_t used_kb = total_kb - free_kb;
        
        *total_mb = total_kb / 1024;
        *used_mb = used_kb / 1024;
        *usage_percent = total_kb > 0 ? (used_kb * 100.0f) / total_kb : 0.0f;
        
        return true;
    }
    return false;
}

/**
 * Get CPU temperature from thermal zone
 */
static float prv_get_cpu_temperature(void) {
    char temp_str[16];
    if (prv_read_file_line("/sys/class/thermal/thermal_zone0/temp", temp_str, sizeof(temp_str))) {
        int temp_millicelsius = atoi(temp_str);
        return temp_millicelsius / 1000.0f;
    }
    return -1.0f;
}

// basic check that the time offset value is at ISO 8601 format
// bug: +12:30 is considered a valid value by this function
static int prv_check_time_offset(char *buffer, int length) {
    int min_index;

    if (length != 3 && length != 5 && length != 6)
        return 0;
    if (buffer[0] != '-' && buffer[0] != '+')
        return 0;
    switch (buffer[1]) {
    case '0':
        if (buffer[2] < '0' || buffer[2] > '9')
            return 0;
        break;
    case '1':
        if (buffer[2] < '0' || (buffer[0] == '-' && buffer[2] > '2') || (buffer[0] == '+' && buffer[2] > '4'))
            return 0;
        break;
    default:
        return 0;
    }
    switch (length) {
    case 3:
        return 1;
    case 5:
        min_index = 3;
        break;
    case 6:
        if (buffer[3] != ':')
            return 0;
        min_index = 4;
        break;
    default:
        // never happen
        return 0;
    }
    if (buffer[min_index] < '0' || buffer[min_index] > '5')
        return 0;
    if (buffer[min_index + 1] < '0' || buffer[min_index + 1] > '9')
        return 0;

    return 1;
}

static uint8_t prv_set_value(lwm2m_data_t *dataP, device_data_t *devDataP) {
    lwm2m_data_t *subTlvP;
    size_t count;
    size_t i;
    // a simple switch structure is used to respond at the specified resource asked
    switch (dataP->id) {
    case RES_O_MANUFACTURER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        prv_detect_hardware();
        lwm2m_data_encode_string(g_manufacturer, dataP);
        return COAP_205_CONTENT;

    case RES_O_MODEL_NUMBER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        prv_detect_hardware();
        lwm2m_data_encode_string(g_model, dataP);
        return COAP_205_CONTENT;

    case RES_O_SERIAL_NUMBER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        prv_detect_hardware();
        lwm2m_data_encode_string(g_serial, dataP);
        return COAP_205_CONTENT;

    case RES_O_FIRMWARE_VERSION:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        prv_detect_hardware();
        if (strlen(g_os_version) > 0) {
            lwm2m_data_encode_string(g_os_version, dataP);
        } else {
            lwm2m_data_encode_string(PRV_FIRMWARE_VERSION, dataP);
        }
        return COAP_205_CONTENT;

    case RES_M_REBOOT:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_FACTORY_RESET:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_AVL_POWER_SOURCES: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 2;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(PRV_POWER_SOURCE_1, subTlvP + i);
                break;
            case 1:
                lwm2m_data_encode_int(PRV_POWER_SOURCE_2, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }

    case RES_O_POWER_SOURCE_VOLTAGE: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 2;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(PRV_POWER_VOLTAGE_1, subTlvP + i);
                break;
            case 1:
                lwm2m_data_encode_int(PRV_POWER_VOLTAGE_2, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }

    case RES_O_POWER_SOURCE_CURRENT: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 2;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(PRV_POWER_CURRENT_1, subTlvP + i);
                break;
            case 1:
                lwm2m_data_encode_int(PRV_POWER_CURRENT_2, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }

    case RES_O_BATTERY_LEVEL:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(devDataP->battery_level, dataP);
        return COAP_205_CONTENT;

    case RES_O_MEMORY_FREE:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(devDataP->free_memory, dataP);
        return COAP_205_CONTENT;

    case RES_M_ERROR_CODE: {
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            count = dataP->value.asChildren.count;
            subTlvP = dataP->value.asChildren.array;
        } else {
            count = 1;
            subTlvP = lwm2m_data_new(count);
            for (i = 0; i < count; i++)
                subTlvP[i].id = i;
            lwm2m_data_encode_instances(subTlvP, count, dataP);
        }

        for (i = 0; i < count; i++) {
            switch (subTlvP[i].id) {
            case 0:
                lwm2m_data_encode_int(devDataP->error, subTlvP + i);
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }

        return COAP_205_CONTENT;
    }
    case RES_O_RESET_ERROR_CODE:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_CURRENT_TIME:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(time(NULL) + devDataP->time, dataP);
        return COAP_205_CONTENT;

    case RES_O_UTC_OFFSET:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(devDataP->time_offset, dataP);
        return COAP_205_CONTENT;

    case RES_O_TIMEZONE:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_TIME_ZONE, dataP);
        return COAP_205_CONTENT;

    case RES_M_BINDING_MODES:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_BINDING_MODE, dataP);
        return COAP_205_CONTENT;

    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_device_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                               lwm2m_object_t *objectP) {
    uint8_t result;
    int i;

    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {RES_O_MANUFACTURER, RES_O_MODEL_NUMBER, RES_O_SERIAL_NUMBER, RES_O_FIRMWARE_VERSION,
                              // E: RES_M_REBOOT,
                              // E: RES_O_FACTORY_RESET,
                              RES_O_AVL_POWER_SOURCES, RES_O_POWER_SOURCE_VOLTAGE, RES_O_POWER_SOURCE_CURRENT,
                              RES_O_BATTERY_LEVEL, RES_O_MEMORY_FREE, RES_M_ERROR_CODE,
                              // E: RES_O_RESET_ERROR_CODE,
                              RES_O_CURRENT_TIME, RES_O_UTC_OFFSET, RES_O_TIMEZONE, RES_M_BINDING_MODES};
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do {
        result = prv_set_value((*dataArrayP) + i, (device_data_t *)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_device_discover(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP,
                                   lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP) {
    uint8_t result;
    int i;

    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    result = COAP_205_CONTENT;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {RES_O_MANUFACTURER,
                              RES_O_MODEL_NUMBER,
                              RES_O_SERIAL_NUMBER,
                              RES_O_FIRMWARE_VERSION,
                              RES_M_REBOOT,
                              RES_O_FACTORY_RESET,
                              RES_O_AVL_POWER_SOURCES,
                              RES_O_POWER_SOURCE_VOLTAGE,
                              RES_O_POWER_SOURCE_CURRENT,
                              RES_O_BATTERY_LEVEL,
                              RES_O_MEMORY_FREE,
                              RES_M_ERROR_CODE,
                              RES_O_RESET_ERROR_CODE,
                              RES_O_CURRENT_TIME,
                              RES_O_UTC_OFFSET,
                              RES_O_TIMEZONE,
                              RES_M_BINDING_MODES};
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    } else {
        for (i = 0; i < *numDataP && result == COAP_205_CONTENT; i++) {
            switch ((*dataArrayP)[i].id) {
            case RES_O_MANUFACTURER:
            case RES_O_MODEL_NUMBER:
            case RES_O_SERIAL_NUMBER:
            case RES_O_FIRMWARE_VERSION:
            case RES_M_REBOOT:
            case RES_O_FACTORY_RESET:
            case RES_O_AVL_POWER_SOURCES:
            case RES_O_POWER_SOURCE_VOLTAGE:
            case RES_O_POWER_SOURCE_CURRENT:
            case RES_O_BATTERY_LEVEL:
            case RES_O_MEMORY_FREE:
            case RES_M_ERROR_CODE:
            case RES_O_RESET_ERROR_CODE:
            case RES_O_CURRENT_TIME:
            case RES_O_UTC_OFFSET:
            case RES_O_TIMEZONE:
            case RES_M_BINDING_MODES:
                break;
            default:
                result = COAP_404_NOT_FOUND;
            }
        }
    }

    return result;
}

static uint8_t prv_device_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                                lwm2m_object_t *objectP, lwm2m_write_type_t writeType) {
    int i;
    uint8_t result;

    /* unused parameter */
    (void)contextP;

    // All write types are treated the same here
    (void)writeType;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    i = 0;

    do {
        /* No multiple instance resources */
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            result = COAP_404_NOT_FOUND;
            continue;
        }

        switch (dataArray[i].id) {
        case RES_O_CURRENT_TIME:
            if (1 == lwm2m_data_decode_int(dataArray + i, &((device_data_t *)(objectP->userData))->time)) {
                ((device_data_t *)(objectP->userData))->time -= time(NULL);
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_UTC_OFFSET:
            if (1 ==
                prv_check_time_offset((char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length)) {
                strncpy(((device_data_t *)(objectP->userData))->time_offset, // NOSONAR
                        (char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                ((device_data_t *)(objectP->userData))->time_offset[dataArray[i].value.asBuffer.length] = 0;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_O_TIMEZONE:
            // ToDo IANA TZ Format
            result = COAP_501_NOT_IMPLEMENTED;
            break;

        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_device_execute(lwm2m_context_t *contextP, uint16_t instanceId, uint16_t resourceId, uint8_t *buffer,
                                  int length, lwm2m_object_t *objectP) {
    /* unused parameter */
    (void)contextP;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    if (length != 0)
        return COAP_400_BAD_REQUEST;

    switch (resourceId) {
    case RES_M_REBOOT:
        fprintf(stdout, "\n\t REBOOT\r\n\n");
        g_reboot = 1;
        return COAP_204_CHANGED;
    case RES_O_FACTORY_RESET:
        fprintf(stdout, "\n\t FACTORY RESET\r\n\n");
        return COAP_204_CHANGED;
    case RES_O_RESET_ERROR_CODE:
        fprintf(stdout, "\n\t RESET ERROR CODE\r\n\n");
        ((device_data_t *)(objectP->userData))->error = 0;
        return COAP_204_CHANGED;
    default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

void display_device_object(lwm2m_object_t *object) {
    device_data_t *data = (device_data_t *)object->userData;
    fprintf(stdout, "  /%u: Device object:\r\n", object->objID);
    if (NULL != data) {
        fprintf(stdout, "    time: %lld, time_offset: %s\r\n", (long long)data->time, data->time_offset);
    }
}

lwm2m_object_t *get_object_device(void) {
    /*
     * The get_object_device function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t *deviceObj;

    deviceObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != deviceObj) {
        memset(deviceObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3 is the standard ID for the mandatory object "Object device".
         */
        deviceObj->objID = LWM2M_DEVICE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        deviceObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != deviceObj->instanceList) {
            memset(deviceObj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(deviceObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library
         * don't need to know the resources of the object, only the server does.
         */
        deviceObj->readFunc = prv_device_read;
        deviceObj->discoverFunc = prv_device_discover;
        deviceObj->writeFunc = prv_device_write;
        deviceObj->executeFunc = prv_device_execute;
        deviceObj->userData = lwm2m_malloc(sizeof(device_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != deviceObj->userData) {
            // Initialize hardware detection
            prv_detect_hardware();
            
            ((device_data_t *)deviceObj->userData)->battery_level = PRV_BATTERY_LEVEL;
            
            // Use real hardware memory info
            uint32_t free_memory_mb = prv_get_free_memory_mb();
            ((device_data_t *)deviceObj->userData)->free_memory = free_memory_mb;
                
            ((device_data_t *)deviceObj->userData)->error = PRV_ERROR_CODE;
            ((device_data_t *)deviceObj->userData)->time = 1367491215;
            strcpy(((device_data_t *)deviceObj->userData)->time_offset, "+01:00"); // NOSONAR
            
            // Show initial system status
            uint64_t disk_used_mb, disk_total_mb;
            float disk_usage_percent;
            if (prv_get_disk_usage(&disk_used_mb, &disk_total_mb, &disk_usage_percent)) {
                printf("[Device] System status - Memory: %d MB free, Disk: %.1f%% used, Temp: %.1f°C\n",
                       (int)free_memory_mb, disk_usage_percent, prv_get_cpu_temperature());
            } else {
                printf("[Device] Initialized with hardware info - Free memory: %d MB\n",
                       (int)free_memory_mb);
            }
        } else {
            lwm2m_free(deviceObj->instanceList);
            lwm2m_free(deviceObj);
            deviceObj = NULL;
        }
    }

    return deviceObj;
}

void free_object_device(lwm2m_object_t *objectP) {
    if (NULL != objectP->userData) {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList) {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }

    lwm2m_free(objectP);
}

uint8_t device_change(lwm2m_data_t *dataArray, lwm2m_object_t *objectP) {
    uint8_t result;

    switch (dataArray->id) {
    case RES_O_BATTERY_LEVEL: {
        int64_t value;
        if (1 == lwm2m_data_decode_int(dataArray, &value)) {
            if ((0 <= value) && (100 >= value)) {
                ((device_data_t *)(objectP->userData))->battery_level = value;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
        } else {
            result = COAP_400_BAD_REQUEST;
        }
    } break;
    case RES_M_ERROR_CODE:
        if (1 == lwm2m_data_decode_int(dataArray, &((device_data_t *)(objectP->userData))->error)) {
            result = COAP_204_CHANGED;
        } else {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    case RES_O_MEMORY_FREE:
        if (1 == lwm2m_data_decode_int(dataArray, &((device_data_t *)(objectP->userData))->free_memory)) {
            result = COAP_204_CHANGED;
        } else {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    default:
        result = COAP_405_METHOD_NOT_ALLOWED;
        break;
    }

    return result;
}

/**
 * Update device object with real hardware information
 */
void device_update_hardware_info(lwm2m_object_t *objectP) {
    if (!objectP || !objectP->userData) {
        return;
    }
    
    device_data_t *deviceData = (device_data_t *)objectP->userData;
    
    // Update free memory with real value
    uint32_t free_memory_mb = prv_get_free_memory_mb();
    deviceData->free_memory = free_memory_mb;
    
    // Get disk usage information
    uint64_t disk_used_mb, disk_total_mb;
    float disk_usage_percent;
    if (prv_get_disk_usage(&disk_used_mb, &disk_total_mb, &disk_usage_percent)) {
        printf("[Device] System health - Memory: %d MB free, Disk: %.1f%% used (%llu/%llu MB)\n", 
               (int)deviceData->free_memory, disk_usage_percent, 
               (unsigned long long)disk_used_mb, (unsigned long long)disk_total_mb);
        
        // Log warning if disk usage is high (>80% for marine deployments)
        if (disk_usage_percent > 80.0f) {
            printf("[Device] WARNING: Disk usage high (%.1f%%) - consider maintenance\n", disk_usage_percent);
        }
    } else {
        printf("[Device] Updated hardware info - Free memory: %d MB\n", 
               (int)deviceData->free_memory);
    }
    
    // Get CPU temperature for marine environment monitoring
    float cpu_temp = prv_get_cpu_temperature();
    if (cpu_temp > 0) {
        printf("[Device] CPU temperature: %.1f°C", cpu_temp);
        if (cpu_temp > 70.0f) {
            printf(" (HIGH - check cooling!)");
        }
        printf("\n");
    }
}
