/*
 * mq2.h
 *
 *  Created on: 11 oct. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_MQ2_H_
#define MAIN_GPIOS_MQ2_H_

#include "driver/adc.h"
#include "recollecter.h"

// ------------------------------------------------- Sensor information --------------------------------------------------------------

// Important defines

#define MQ2_N_GPIOS								1
#define MQ2_N_VALUES							1
#define MQ2_N_ADDITIONAL_PARAMS					0

#define MQ2_TIME_TO_UPDATE_DATA					500	// 100 = 1 second
#define MQ2_STABILIZED_TICKS					18000 / MQ2_TIME_TO_UPDATE_DATA

// LCD related defines (1 for each value)

#define MQ2_SHOW_SMOKE_GAS_PERCENTAGE_ON_LCD	true

// Additional Defines

#define MQ2_LOW_V								1000
#define MQ2_HIGH_V								4095

static const adc_bits_width_t mq2_width 		= ADC_WIDTH_BIT_12;
static const adc_atten_t mq2_atten 				= ADC_ATTEN_DB_11;

#define MQ2_N_SAMPLES   						64	//Used for minimizing noise

// Structure for sensor GPIOS

typedef struct {
	int a0;
} mq2_gpios_t;

// Structure for sensor data

typedef struct {
	float smoke_gas_percentage;
} mq2_data_t;

// Structure for sensor alerts

typedef struct {
	bool smoke_gas_alert;
	int smoke_gas_ticks_to_alert;
	float smoke_gas_upper_threshold;
	float smoke_gas_lower_threshold;
} mq2_alerts_t;

// --------------------------------------------------- Sensor management -----------------------------------------------------------

/*
 * Initializes MQ2 peripheral
 */
void mq2_init(void);

/**
 * Add a new MQ2 sensor
 *
 * NOTE: return 1 when it happens successfully, -1 in other cases
 */
int mq2_add_sensor(int* gpios, union sensor_value_u* parameters, char* reason);

/**
 * Delete a MQ2 sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_delete_sensor(int pos, char* reason);

// ------------------------------------------------------- Sets --------------------------------------------------------------------

/**
 * Change GPIOS of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_gpios(int pos, int* gpios, char* reason);

/**
 * Change parameters of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_parameters(int pos, union sensor_value_u* parameters, char* reason);

/**
 * Change location of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_location(int pos, char* location, char* reason);

/**
 * Change the alert values
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold, char* reason);

// ------------------------------------------------------- Gets --------------------------------------------------------------------

/**
 * Gets data of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_data_t* mq2_get_sensors_data(int* number_of_sensors);

/**
 * Gets GPIOS of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_gpios_info_t* mq2_get_sensors_gpios(int* number_of_sensors);

/**
 * Gets additional parameters of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_additional_parameters_info_t* mq2_get_sensors_additional_parameters(int* number_of_sensors);


#endif /* MAIN_GPIOS_MQ2_H_ */
