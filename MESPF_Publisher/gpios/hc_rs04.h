/*
 * hc_rs04.h
 *
 *  Created on: 3 nov. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_HC_RS04_H_
#define MAIN_GPIOS_HC_RS04_H_

#include "recollecter.h"

// ------------------------------------------------- Sensor information --------------------------------------------------------------

#define HC_RS04_N_GPIOS						2

#define HC_RS04_N_VALUES					1

#define HC_RS04_N_ADDITIONAL_PARAMS			2

#define PING_TIMEOUT 						6000

#define ROUNDTRIP 							58

#define MAX_SENSE_DISTANCE					400 		// Supposed to be around that value (centimeters). Other models can go even further

#define HC_RS04_TIME_TO_UPDATE_DATA			500 		// 100 = 1 second

// Structure for sensor additional parameters

typedef struct {
	int distance_between_sensor_and_tank; 				// Gap between the sensor and the top of the water tank (centimeters)
	int tank_length; 									// Up to bottom length of the water tank (centimeters)
} hc_rs04_additional_params_t;

// Structure for sensor GPIOS

typedef struct {
	int trig;
	int echo;
} hc_rs04_gpios_t;

// Structure for sensor data

typedef struct {
	float water_level_percentage;
} hc_rs04_data_t;

// Value related to the LCD

#define HC_RS04_SHOW_WATER_LEVEL_ON_LCD		true

// --------------------------------------------------- Sensor management -----------------------------------------------------------

/*
 * Startup HC_RS04 with the information previously saved (NVS)
 */
void hc_rs04_startup(void);

/*
 * Initializes HC_RS04 peripheral
 */
void hc_rs04_init(void);

/*
 * Disables HC_RS04 peripheral
 */
void hc_rs04_destroy(void);

/**
 * Add a new HC_RS04 sensor
 *
 * NOTE: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_add_sensor(int* gpios, union sensor_value_u* parameters);

/**
 * Delete a HC_RS04 sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_delete_sensor(int pos);

// ------------------------------------------------------- Sets --------------------------------------------------------------------

/**
 * Change GPIOS of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_gpios(int pos, int* gpios);

/**
 * Change parameters of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_parameters(int pos, union sensor_value_u* parameters);

/**
 * Change the alert values
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold);

// ------------------------------------------------------- Gets --------------------------------------------------------------------

/**
 * Gets data of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_data_t* hc_rs04_get_sensors_data(int* number_of_sensors);

/**
 * Gets GPIOS of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_gpios_info_t* hc_rs04_get_sensors_gpios(int* number_of_sensors);

/**
 * Gets additional parameters of all sensors of this type
 *
 * Note: in case of error (or it is a sensor with no additional parameters), return NULL pointer
 */
sensor_additional_parameters_info_t* hc_rs04_get_sensors_additional_parameters(int* number_of_sensors);

#endif /* MAIN_GPIOS_HC_RS04_H_ */
