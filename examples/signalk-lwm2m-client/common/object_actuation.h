/*******************************************************************************
 *
 * Copyright (c) 2025 Marine IoT Systems
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * Contributors:
 *    Marine IoT Systems - SignalK LwM2M Actuation implementation
 *
 *******************************************************************************/

#ifndef OBJECT_ACTUATION_H_
#define OBJECT_ACTUATION_H_

#include "liblwm2m.h"

/*
 * OMA LwM2M Actuation Object (3306)
 * 
 * Resources:
 * 5850 - On/Off (Boolean, R/W)
 * 5851 - Dimmer (Integer 0-100, R/W, Optional)
 * 5701 - Units (String, R)
 * 5750 - Application Type (String, R)
 */

/**
 * Creates the Actuation Object for marine switch control
 * @return Pointer to the created LwM2M object
 */
lwm2m_object_t *get_actuation_object(void);

/**
 * Frees the Actuation Object
 * @param objectP Pointer to the object to free
 */
void free_actuation_object(lwm2m_object_t *objectP);

/**
 * Update switch state from SignalK data
 * @param instance_id Instance ID of the switch
 * @param state Boolean state (true = on, false = off)
 * @param lwm2mH LwM2M context for notifications
 */
void actuation_update_state(uint16_t instance_id, bool state, lwm2m_context_t *lwm2mH);

/**
 * Update dimmer value from SignalK data
 * @param instance_id Instance ID of the dimmer
 * @param dimmer_value Dimmer value (0-100)
 * @param lwm2mH LwM2M context for notifications
 */
void actuation_update_dimmer(uint16_t instance_id, int dimmer_value, lwm2m_context_t *lwm2mH);

#endif /* OBJECT_ACTUATION_H_ */
