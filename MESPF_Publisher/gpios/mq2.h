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

#define MQ2_N_GPIOS								1

#define MQ2_N_VALUES							1

#define MQ2_N_ADDITIONAL_PARAMS					0

#define MQ2_LOW_V								1000

#define MQ2_HIGH_V								4095

#define MQ2_TIME_TO_UPDATE_DATA					500	// 100 = 1 second

// Structure for sensor GPIOS

typedef struct {
	int a0;
} mq2_gpios_t;

// Structure for sensor data

typedef struct {
	float smoke_gas_percentage;
} mq2_data_t;

// Value related to the LCD

#define MQ2_SHOW_SMOKE_GAS_PERCENTAGE_ON_LCD	true

// Lecture properties

static const adc_bits_width_t mq2_width 		= ADC_WIDTH_BIT_12;
static const adc_atten_t mq2_atten 				= ADC_ATTEN_DB_11;

#define MQ2_N_SAMPLES   						64	//Used for minimizing noise

// --------------------------------------------------- Sensor management -----------------------------------------------------------

/*
 * Startup MQ2 with the information previously saved (NVS)
 */
void mq2_startup(void);

/*
 * Initializes MQ2 peripheral
 */
void mq2_init(void);

/*
 * Disables MQ2 peripheral
 */
void mq2_destroy(void);

/**
 * Add a new MQ2 sensor
 *
 * NOTE: return 1 when it happens successfully, -1 in other cases
 */
int mq2_add_sensor(int* gpios, union sensor_value_u* parameters);

/**
 * Delete a MQ2 sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_delete_sensor(int pos);

// ------------------------------------------------------- Sets --------------------------------------------------------------------

/**
 * Change GPIOS of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_gpios(int pos, int* gpios);

/**
 * Change parameters of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_parameters(int pos, union sensor_value_u* parameters);

/**
 * Change the alert values
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int mq2_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold);

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
 * Note: in case of error (or it is a sensor with no additional parameters), return NULL pointer
 */
sensor_additional_parameters_info_t* mq2_get_sensors_additional_parameters(int* number_of_sensors);


#endif /* MAIN_GPIOS_MQ2_H_ */
