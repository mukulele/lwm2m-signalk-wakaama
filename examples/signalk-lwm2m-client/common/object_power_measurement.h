#ifndef OBJECT_POWER_MEASUREMENT_H
#define OBJECT_POWER_MEASUREMENT_H

#include "liblwm2m.h"

/**
 * Power Measurement Object (3305)
 * 
 * Marine electrical system monitoring using OMA-defined Power Measurement object.
 * Instances are allocated as follows:
 * 
 * 0-9:   Battery bank monitoring (voltage/current/power)
 * 10-19: Generation source monitoring (solar/wind/alternator) 
 * 20-29: Load circuit monitoring (individual circuits)
 * 30-39: AC system monitoring (inverters/chargers)
 * 40-49: System-wide measurements (total consumption/generation)
 */

lwm2m_object_t * get_power_measurement_object(void);
void free_power_measurement_object(lwm2m_object_t * objectP);

/**
 * Update power measurement value from SignalK data
 * @param objectP Power measurement object
 * @param instanceId Instance to update (0-49)
 * @param value Measured value (voltage, current, power)
 * @param units Measurement units ("V", "A", "W", "kW")
 * @param appType Application description ("House Battery Voltage", etc.)
 */
void power_measurement_update_value(lwm2m_object_t * objectP, uint16_t instanceId, 
                                   double value, const char* units, const char* appType);

#endif /* OBJECT_POWER_MEASUREMENT_H */
