#ifndef OBJECT_ENERGY_H
#define OBJECT_ENERGY_H

#include "liblwm2m.h"

/**
 * Energy Object (3331)
 * 
 * Marine energy tracking using OMA-defined Energy object.
 * Tracks cumulative energy consumption and generation.
 * 
 * Instance allocation:
 * 0: Solar generation energy
 * 1: Shore power consumption  
 * 2: House load consumption
 * 3: Engine charging energy
 * 4: Wind generation energy
 * 5: Battery bank energy flow
 */

lwm2m_object_t * get_energy_object(void);
void free_energy_object(lwm2m_object_t * objectP);

/**
 * Update energy measurement from SignalK data
 * @param objectP Energy object
 * @param instanceId Instance to update (0-9)
 * @param cumulativeWh Cumulative energy in Wh
 * @param currentW Current power in W
 * @param appType Application description
 */
void energy_update_value(lwm2m_object_t * objectP, uint16_t instanceId, 
                        double cumulativeWh, double currentW, const char* appType);

#endif /* OBJECT_ENERGY_H */
