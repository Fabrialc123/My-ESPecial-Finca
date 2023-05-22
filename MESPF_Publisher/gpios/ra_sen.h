/*
 * ra_sen.h
 *
 *  Created on: 22 may. 2023
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_RA_SEN_H_
#define MAIN_GPIOS_RA_SEN_H_

#include "driver/adc.h"
#include "recollecter.h"

// ------------------------------------------------- Sensor information --------------------------------------------------------------

// Important defines

#define RA_SEN_N_GPIOS							1
#define RA_SEN_N_VALUES							1
#define RA_SEN_N_ADDITIONAL_PARAMS				0

#define RA_SEN_TIME_TO_UPDATE_DATA				500	// 100 = 1 second
#define RA_SEN_STABILIZED_TICKS					18000 / RA_SEN_TIME_TO_UPDATE_DATA

// LCD related defines (1 for each value)

#define RA_SEN_SHOW_WET_PERCENTAGE_ON_LCD	true

// Additional Defines

#define RA_SEN_LOW_V							4095
#define RA_SEN_HIGH_V							1300

static const adc_bits_width_t ra_sen_width 		= ADC_WIDTH_BIT_12;
static const adc_atten_t ra_sen_atten	 		= ADC_ATTEN_DB_11;

#define RA_SEN_N_SAMPLES   						64	//Used for minimizing noise

// Structure for sensor GPIOS

typedef struct {
	int a0;
} ra_sen_gpios_t;

// Structure for sensor data

typedef struct {
	float wet_percentage;
} ra_sen_data_t;

// Structure for sensor alerts

typedef struct {
	bool wet_alert;
	int wet_ticks_to_alert;
	float wet_upper_threshold;
	float wet_lower_threshold;
} ra_sen_alerts_t;

// --------------------------------------------------- Sensor management -----------------------------------------------------------

/*
 * Initializes RA_SEN peripheral
 */
void ra_sen_init(void);

/**
 * Add a new RA_SEN sensor
 *
 * NOTE: return 1 when it happens successfully, -1 in other cases
 */
int ra_sen_add_sensor(int* gpios, union sensor_value_u* parameters, char* reason);

/**
 * Delete a RA_SEN sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int ra_sen_delete_sensor(int pos, char* reason);

// ------------------------------------------------------- Sets --------------------------------------------------------------------

/**
 * Change GPIOS of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int ra_sen_set_gpios(int pos, int* gpios, char* reason);

/**
 * Change parameters of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int ra_sen_set_parameters(int pos, union sensor_value_u* parameters, char* reason);

/**
 * Change location of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int ra_sen_set_location(int pos, char* location, char* reason);

/**
 * Change the alert values
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int ra_sen_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold, char* reason);

// ------------------------------------------------------- Gets --------------------------------------------------------------------

/**
 * Gets data of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_data_t* ra_sen_get_sensors_data(int* number_of_sensors);

/**
 * Gets GPIOS of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_gpios_info_t* ra_sen_get_sensors_gpios(int* number_of_sensors);

/**
 * Gets additional parameters of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_additional_parameters_info_t* ra_sen_get_sensors_additional_parameters(int* number_of_sensors);

#endif /* MAIN_GPIOS_RA_SEN_H_ */
