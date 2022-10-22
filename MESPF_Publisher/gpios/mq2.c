/*
 * mq2.c
 *
 *  Created on: 11 oct. 2022
 *      Author: Kike
 */

#include "gpios/mq2.h"

#include <stdbool.h>

#include "esp_log.h"
#include "string.h"

static const char TAG[] = "mq2";

bool g_mq2_initialized 	= false;

static sensor_data_t sensor_data;

void mq2_init(void){

	int number_of_values = 1;

	adc1_config_width(width);
	adc1_config_channel_atten(channel, atten);

	strcpy(sensor_data.sensorName, "MQ2");

	sensor_data.valuesLen = number_of_values;

	sensor_data.sensor_values = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(sensor_data.sensor_values[0].valueName, "S/G");
	sensor_data.sensor_values[0].sensor_value_type = INTEGER;

	g_mq2_initialized = true;
}

sensor_data_t mq2_get_sensor_data(void){

	int read_raw = 0;

	if(g_mq2_initialized){

        for (int i = 0; i < N_SAMPLES; i++) {
        	read_raw += adc1_get_raw(channel);
        }

        sensor_data.sensor_values[0].sensor_value.ival = read_raw / N_SAMPLES;
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing the ADC channel");
	}

	return sensor_data;
}
