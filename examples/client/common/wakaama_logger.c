#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "wakaama_logger.h"

/**
 * Custom logging function for Wakaama.
 * Adds timestamps to log messages and prints them to stdout.
 */
void lwm2m_custom_log_handler(const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Get current timestamp
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    // Print timestamp before log message
    printf("[%02d:%02d:%02d] ", local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
    
    // Print the log message
    vprintf(format, args);
    printf("\n");  // Ensure newline at the end

    va_end(args);
}
