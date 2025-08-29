#ifndef HARDWARE_INFO_H
#define HARDWARE_INFO_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/**
 * @file hardware_info.h
 * @brief Hardware detection and system information for marine IoT devices
 * 
 * Provides hardware identification and system health monitoring specifically
 * for Raspberry Pi based marine IoT deployments.
 */

/**
 * Hardware information structure
 */
typedef struct {
    char manufacturer[64];      ///< Hardware manufacturer (e.g., "Raspberry Pi Foundation")
    char model[64];            ///< Device model (e.g., "Raspberry Pi 4 Model B")
    char serial_number[32];    ///< Hardware serial number
    char cpu_architecture[32]; ///< CPU architecture (e.g., "aarch64", "armv7l")
    uint64_t total_memory_kb;  ///< Total system memory in KB
    char hardware_revision[16]; ///< Hardware revision code
} hardware_info_t;

/**
 * Operating system information structure
 */
typedef struct {
    char kernel_version[128];   ///< Linux kernel version
    char kernel_release[64];    ///< Kernel release string
    char os_name[64];          ///< Operating system name
    char os_version[64];       ///< OS version/distribution
    char hostname[64];         ///< System hostname
} os_info_t;

/**
 * System health information structure
 */
typedef struct {
    float cpu_temperature;     ///< CPU temperature in Celsius
    uint64_t memory_used_kb;   ///< Used memory in KB
    uint64_t memory_free_kb;   ///< Free memory in KB
    uint64_t disk_used_kb;     ///< Used disk space in KB
    uint64_t disk_free_kb;     ///< Free disk space in KB
    uint64_t disk_total_kb;    ///< Total disk space in KB
    time_t last_update;        ///< Timestamp of last update
} system_health_t;

/**
 * Initialize hardware information detection
 * @return true on success, false on failure
 */
bool hardware_info_init(void);

/**
 * Get static hardware information
 * This information is detected once and cached
 * @param info Pointer to hardware_info_t structure to fill
 * @return true on success, false on failure
 */
bool hardware_info_get_static(hardware_info_t *info);

/**
 * Get operating system information
 * This information may change over time (kernel updates, etc.)
 * @param info Pointer to os_info_t structure to fill
 * @return true on success, false on failure
 */
bool hardware_info_get_os(os_info_t *info);

/**
 * Get current system health information
 * This information is dynamic and should be updated regularly
 * @param health Pointer to system_health_t structure to fill
 * @return true on success, false on failure
 */
bool hardware_info_get_health(system_health_t *health);

/**
 * Get hardware manufacturer string
 * @return Manufacturer string (cached)
 */
const char* hardware_info_get_manufacturer(void);

/**
 * Get hardware model string
 * @return Model string (cached)
 */
const char* hardware_info_get_model(void);

/**
 * Get hardware serial number
 * @return Serial number string (cached)
 */
const char* hardware_info_get_serial(void);

/**
 * Get current kernel version
 * @return Kernel version string
 */
const char* hardware_info_get_kernel_version(void);

/**
 * Get current memory usage percentage
 * @return Memory usage percentage (0-100)
 */
float hardware_info_get_memory_usage_percent(void);

/**
 * Get current disk usage percentage
 * @return Disk usage percentage (0-100)
 */
float hardware_info_get_disk_usage_percent(void);

/**
 * Get current CPU temperature
 * @return CPU temperature in Celsius
 */
float hardware_info_get_cpu_temperature(void);

/**
 * Get free memory in MB
 * @return Free memory in megabytes
 */
uint32_t hardware_info_get_free_memory_mb(void);

/**
 * Cleanup hardware information system
 */
void hardware_info_cleanup(void);

#endif // HARDWARE_INFO_H
