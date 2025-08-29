#include "hardware_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <errno.h>

/**
 * @file hardware_info.c
 * @brief Implementation of hardware detection and system information
 * 
 * Provides comprehensive hardware and system information for marine IoT
 * devices, specifically optimized for Raspberry Pi platforms.
 */

// Static hardware information (cached at initialization)
static hardware_info_t g_hardware_info;
static bool g_hardware_initialized = false;

// Dynamic system health information
static system_health_t g_system_health;

/**
 * Read a single line from a file
 */
static bool read_file_line(const char *filename, char *buffer, size_t buffer_size) {
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
static bool get_cpuinfo_field(const char *field_name, char *buffer, size_t buffer_size) {
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
 * Detect Raspberry Pi model from device tree
 */
static bool detect_rpi_model(char *model_buffer, size_t buffer_size) {
    // Try device tree model first
    if (read_file_line("/proc/device-tree/model", model_buffer, buffer_size)) {
        return true;
    }
    
    // Fallback to cpuinfo Hardware field
    if (get_cpuinfo_field("Hardware", model_buffer, buffer_size)) {
        return true;
    }
    
    // Last resort - try to detect from revision
    char revision[32];
    if (get_cpuinfo_field("Revision", revision, sizeof(revision))) {
        // Basic Pi model detection from revision codes
        if (strstr(revision, "a02082") || strstr(revision, "a22082")) {
            strncpy(model_buffer, "Raspberry Pi 3 Model B", buffer_size - 1);
        } else if (strstr(revision, "a020d3")) {
            strncpy(model_buffer, "Raspberry Pi 3 Model B+", buffer_size - 1);
        } else if (strstr(revision, "a03111") || strstr(revision, "b03111") || strstr(revision, "c03111")) {
            strncpy(model_buffer, "Raspberry Pi 4 Model B", buffer_size - 1);
        } else {
            snprintf(model_buffer, buffer_size, "Raspberry Pi (Rev: %s)", revision);
        }
        model_buffer[buffer_size - 1] = '\0';
        return true;
    }
    
    return false;
}

/**
 * Get system memory information
 */
static uint64_t get_total_memory_kb(void) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return (uint64_t)(si.totalram * si.mem_unit) / 1024;
    }
    return 0;
}

/**
 * Get current memory usage
 */
static bool get_memory_usage(uint64_t *used_kb, uint64_t *free_kb) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        *free_kb = (uint64_t)(si.freeram * si.mem_unit) / 1024;
        *used_kb = (uint64_t)((si.totalram - si.freeram) * si.mem_unit) / 1024;
        return true;
    }
    return false;
}

/**
 * Get disk usage for root filesystem
 */
static bool get_disk_usage(uint64_t *used_kb, uint64_t *free_kb, uint64_t *total_kb) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0) {
        *total_kb = (uint64_t)(vfs.f_blocks * vfs.f_frsize) / 1024;
        *free_kb = (uint64_t)(vfs.f_bavail * vfs.f_frsize) / 1024;
        *used_kb = *total_kb - *free_kb;
        return true;
    }
    return false;
}

/**
 * Get CPU temperature from thermal zone
 */
static float get_cpu_temperature(void) {
    char temp_str[16];
    if (read_file_line("/sys/class/thermal/thermal_zone0/temp", temp_str, sizeof(temp_str))) {
        int temp_millicelsius = atoi(temp_str);
        return temp_millicelsius / 1000.0f;
    }
    return -1.0f;
}

bool hardware_info_init(void) {
    if (g_hardware_initialized) {
        return true;
    }
    
    memset(&g_hardware_info, 0, sizeof(g_hardware_info));
    
    // Set manufacturer (always Raspberry Pi Foundation for Pi devices)
    strncpy(g_hardware_info.manufacturer, "Raspberry Pi Foundation", sizeof(g_hardware_info.manufacturer) - 1);
    
    // Detect model
    if (!detect_rpi_model(g_hardware_info.model, sizeof(g_hardware_info.model))) {
        strncpy(g_hardware_info.model, "Unknown Raspberry Pi", sizeof(g_hardware_info.model) - 1);
    }
    
    // Get serial number
    if (!get_cpuinfo_field("Serial", g_hardware_info.serial_number, sizeof(g_hardware_info.serial_number))) {
        strncpy(g_hardware_info.serial_number, "Unknown", sizeof(g_hardware_info.serial_number) - 1);
    }
    
    // Get CPU architecture
    struct utsname uts;
    if (uname(&uts) == 0) {
        strncpy(g_hardware_info.cpu_architecture, uts.machine, sizeof(g_hardware_info.cpu_architecture) - 1);
    } else {
        strncpy(g_hardware_info.cpu_architecture, "unknown", sizeof(g_hardware_info.cpu_architecture) - 1);
    }
    
    // Get total memory
    g_hardware_info.total_memory_kb = get_total_memory_kb();
    
    // Get hardware revision
    if (!get_cpuinfo_field("Revision", g_hardware_info.hardware_revision, sizeof(g_hardware_info.hardware_revision))) {
        strncpy(g_hardware_info.hardware_revision, "unknown", sizeof(g_hardware_info.hardware_revision) - 1);
    }
    
    g_hardware_initialized = true;
    printf("[Hardware] Initialized: %s %s (Serial: %s)\n", 
           g_hardware_info.manufacturer, g_hardware_info.model, g_hardware_info.serial_number);
    
    return true;
}

bool hardware_info_get_static(hardware_info_t *info) {
    if (!g_hardware_initialized) {
        if (!hardware_info_init()) {
            return false;
        }
    }
    
    if (info) {
        memcpy(info, &g_hardware_info, sizeof(hardware_info_t));
        return true;
    }
    
    return false;
}

bool hardware_info_get_os(os_info_t *info) {
    if (!info) return false;
    
    memset(info, 0, sizeof(os_info_t));
    
    struct utsname uts;
    if (uname(&uts) == 0) {
        strncpy(info->kernel_version, uts.version, sizeof(info->kernel_version) - 1);
        strncpy(info->kernel_release, uts.release, sizeof(info->kernel_release) - 1);
        strncpy(info->os_name, uts.sysname, sizeof(info->os_name) - 1);
        strncpy(info->hostname, uts.nodename, sizeof(info->hostname) - 1);
        
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
                            strncpy(info->os_version, start, sizeof(info->os_version) - 1);
                        }
                    }
                    break;
                }
            }
            fclose(os_file);
        }
        
        if (strlen(info->os_version) == 0) {
            strncpy(info->os_version, "Linux", sizeof(info->os_version) - 1);
        }
        
        return true;
    }
    
    return false;
}

bool hardware_info_get_health(system_health_t *health) {
    if (!health) return false;
    
    memset(health, 0, sizeof(system_health_t));
    
    // Get memory usage
    get_memory_usage(&health->memory_used_kb, &health->memory_free_kb);
    
    // Get disk usage
    get_disk_usage(&health->disk_used_kb, &health->disk_free_kb, &health->disk_total_kb);
    
    // Get CPU temperature
    health->cpu_temperature = get_cpu_temperature();
    
    health->last_update = time(NULL);
    
    return true;
}

const char* hardware_info_get_manufacturer(void) {
    if (!g_hardware_initialized) hardware_info_init();
    return g_hardware_info.manufacturer;
}

const char* hardware_info_get_model(void) {
    if (!g_hardware_initialized) hardware_info_init();
    return g_hardware_info.model;
}

const char* hardware_info_get_serial(void) {
    if (!g_hardware_initialized) hardware_info_init();
    return g_hardware_info.serial_number;
}

const char* hardware_info_get_kernel_version(void) {
    static os_info_t os_info;
    static time_t last_update = 0;
    time_t now = time(NULL);
    
    // Update OS info every 60 seconds
    if (now - last_update > 60) {
        hardware_info_get_os(&os_info);
        last_update = now;
    }
    
    return os_info.kernel_release;
}

float hardware_info_get_memory_usage_percent(void) {
    system_health_t health;
    if (hardware_info_get_health(&health)) {
        uint64_t total = health.memory_used_kb + health.memory_free_kb;
        if (total > 0) {
            return (health.memory_used_kb * 100.0f) / total;
        }
    }
    return 0.0f;
}

float hardware_info_get_disk_usage_percent(void) {
    system_health_t health;
    if (hardware_info_get_health(&health)) {
        if (health.disk_total_kb > 0) {
            return (health.disk_used_kb * 100.0f) / health.disk_total_kb;
        }
    }
    return 0.0f;
}

float hardware_info_get_cpu_temperature(void) {
    return get_cpu_temperature();
}

uint32_t hardware_info_get_free_memory_mb(void) {
    system_health_t health;
    if (hardware_info_get_health(&health)) {
        return (uint32_t)(health.memory_free_kb / 1024);
    }
    return 0;
}

void hardware_info_cleanup(void) {
    g_hardware_initialized = false;
    memset(&g_hardware_info, 0, sizeof(g_hardware_info));
    memset(&g_system_health, 0, sizeof(g_system_health));
}
