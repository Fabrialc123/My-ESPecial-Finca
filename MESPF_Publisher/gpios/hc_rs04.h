/*
 * hc_rs04.h
 *
 *  Created on: 3 nov. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_HC_RS04_H_
#define MAIN_GPIOS_HC_RS04_H_

#include "recollecter.h"

#define HC_RS04_ECHO_GPIO					14
#define HC_RS04_TRIG_GPIO					12

#define MAX_SENSE_DISTANCE					400 		// Supposed to be around that value (centimeters). Other models can go even further

#define DISTANCE_BETWEEN_SENSOR_AND_TANK	5			// Gap between the sensor and the top of the water tank (centimeters)
#define TANK_LENGTH							100			// Up to bottom length of the water tank (centimeters)

// [Note] The sum of the 2 previous values musn't be higher than MAX_SENSE_DISTANCE or the sensor will fail

#define HC_RS04_TIME_TO_UPDATE_DATA			100 * 5 	// 1 second = 100 ticks

// Value related to the LCD

#define HC_RS04_SHOW_WATER_LEVEL_ON_LCD		true

// Values related to the alerts

#define HC_RS04_ALERT_WATER_LEVEL			true
#define HC_RS04_WATER_LEVEL_TICKS_TO_ALERT	3
#define HC_RS04_WATER_LEVEL_UPPER_THRESHOLD	70.0
#define HC_RS04_WATER_LEVEL_LOWER_THRESHOLD	0.0

/*
 * Initializes HC_RS04 peripheral
 */
void hc_rs04_init(void);

/**
 * Gets data
 */
sensor_data_t hc_rs04_get_sensor_data(void);

#endif /* MAIN_GPIOS_HC_RS04_H_ */
