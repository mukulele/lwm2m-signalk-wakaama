/*******************************************************************************
 * Bridge Object Mock Header for Unit Testing
 * 
 * Defines the bridge object API for isolated unit testing
 *******************************************************************************/

#ifndef BRIDGE_OBJECT_MOCK_H
#define BRIDGE_OBJECT_MOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Core bridge object API */
void bridge_init(void);
int bridge_register(uint16_t objId, uint16_t instId, uint16_t resId, const char *signalK_path);
void bridge_update(const char *signalK_path, const char *value);

/* Additional utility functions for testing */
int bridge_get_registry_count(void);
int bridge_find_resource(const char *signalK_path, uint16_t *objId, uint16_t *instId, uint16_t *resId);
const char* bridge_get_last_value(const char *signalK_path);
void bridge_print_registry(void);

#ifdef __cplusplus
}
#endif

#endif /* BRIDGE_OBJECT_MOCK_H */
