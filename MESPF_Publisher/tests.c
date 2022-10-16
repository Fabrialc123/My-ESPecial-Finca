/*
 * tests.c
 *
 *  Created on: 26 sept. 2022
 *      Author: fabri
 */
#include "esp_log.h"

#include "mqtt_app.h"
#include "recollecter.h"
#include <string.h>

static const char TAG[] = "TEST";

static sensor_data_t sensor_data;

sensor_data_t TEST_recollecter (void){

	/**
	 *
	 * Here you update peripherals data
	 *
	 * IMPORTANT!!! Here i'm not doing it but first of all check that you have initialized the sensor (e.g with a global boolean variable)
	 *
	 */

	//First peripheral
	sensor_data.sensor_values[0].sensor_value.ival = 1111;

	//Second peripheral
	sensor_data.sensor_values[1].sensor_value.fval = 22.22;

	//Third peripheral
	strcpy(sensor_data.sensor_values[2].sensor_value.cval , "333.3");

	return sensor_data;
}

void TEST_init(void){

	int number_of_values = 3;

	//Sensor name
	strcpy(sensor_data.sensorName, "TESTING");

	//Sensor values length
	sensor_data.valuesLen = number_of_values;

	//Sensor peripherals
	sensor_data.sensor_values = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	//First peripheral
	strcpy(sensor_data.sensor_values[0].valueName,"INTEGERTEST");
	sensor_data.sensor_values[0].sensor_value_type = INTEGER;

	//Second peripheral
	strcpy(sensor_data.sensor_values[1].valueName,"FLOATTEST");
	sensor_data.sensor_values[1].sensor_value_type = FLOAT;

	//Third peripheral
	strcpy(sensor_data.sensor_values[2].valueName,"STRINGTEST");
	sensor_data.sensor_values[2].sensor_value_type = STRING;
}

void TEST_mqtt_app_recollect(void){
	int i, aux;

	for (i = 0; i < 1 ;i++){ // Can be changed to RECOLLECTER_SIZE
	 aux = register_recollecter(&TEST_recollecter);
	 ESP_LOGE(TAG, "registered %d recollecter with result = %d", i,aux);
	}
}

void TEST_mqtt_app_refresh(void){
	mqtt_app_refresh_TEST();
}
