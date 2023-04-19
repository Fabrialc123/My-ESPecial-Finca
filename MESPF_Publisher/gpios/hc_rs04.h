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

// Important defines

#define HC_RS04_N_GPIOS						2
#define HC_RS04_N_VALUES					1
#define HC_RS04_N_ADDITIONAL_PARAMS			2

#define HC_RS04_TIME_TO_UPDATE_DATA			500 		// 100 = 1 second
#define HC_RS04_STABILIZED_TICKS			18000 / HC_RS04_TIME_TO_UPDATE_DATA

// LCD related defines (1 for each value)

#define HC_RS04_SHOW_WATER_LEVEL_ON_LCD		true

// Additional Defines

#define MAX_SENSE_DISTANCE					400 		// Supposed to be around that value (centimeters). Other models can go even further
#define PING_TIMEOUT 						6000
#define ROUNDTRIP 							58

// Structure for sensor additional parameters

typedef struct {
	int distance_between_sensor_and_tank; 				// Gap between the sensor and the top of the tank (centimeters)
	int tank_depth; 									// Water tank depth (centimeters)
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

// Structure for sensor alerts

typedef struct {
	bool water_level_alert;
	int water_level_ticks_to_alert;
	float water_level_upper_threshold;
	float water_level_lower_threshold;
} hc_rs04_alerts_t;

// --------------------------------------------------- Sensor management -----------------------------------------------------------

/*
 * Initializes HC_RS04 peripheral
 */
void hc_rs04_init(void);

/**
 * Add a new HC_RS04 sensor
 *
 * NOTE: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_add_sensor(int* gpios, union sensor_value_u* parameters, char* reason);

/**
 * Delete a HC_RS04 sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_delete_sensor(int pos, char* reason);

// ------------------------------------------------------- Sets --------------------------------------------------------------------

/**
 * Change GPIOS of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_gpios(int pos, int* gpios, char* reason);

/**
 * Change parameters of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_parameters(int pos, union sensor_value_u* parameters, char* reason);

/**
 * Change location of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_location(int pos, char* location, char* reason);

/**
 * Change the alert values
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int hc_rs04_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold, char* reason);

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
 * Note: in case of error, return NULL pointer
 */
sensor_additional_parameters_info_t* hc_rs04_get_sensors_additional_parameters(int* number_of_sensors);

#endif /* MAIN_GPIOS_HC_RS04_H_ */
