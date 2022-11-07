/*
 * tests.c
 *
 *  Created on: 26 sept. 2022
 *      Author: fabri
 */
#include <mqtt/mqtt_app.h>
#include "esp_log.h"

#include "recollecter.h"
#include <string.h>

#include "mqtt/mqtt_commands.h"

static const char TAG[] = "TEST";

sensor_data_t TEST_recollecter (void){
	sensor_data_t aux;
	sensor_value_t *aux2;
	int number_of_values = 3;

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "TESTING");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;
	aux.sensor_values[0].sensor_value_type = INTEGER;
	strcpy(aux.sensor_values[0].valueName,"INTEGERTEST");
	aux.sensor_values[0].sensor_value.ival = 1111;

	aux.sensor_values[1].sensor_value_type = FLOAT;
	strcpy(aux.sensor_values[1].valueName,"FLOATTEST");
	aux.sensor_values[1].sensor_value.fval = 22.22;

	aux.sensor_values[2].sensor_value_type = STRING;
	strcpy(aux.sensor_values[2].valueName,"STRINGTEST");
	strcpy(aux.sensor_values[2].sensor_value.cval , "CHAR");

	return aux;
}

void TEST_recollecter_register_recollecter(void){
	int i, aux;

	for (i = 0; i < 1 ;i++){ // Can be changed to RECOLLECTER_SIZE
	 aux = register_recollecter(&TEST_recollecter);
	 ESP_LOGE(TAG, "registered %d recollecter with result = %d", i,aux);
	}
}

void TEST_mqtt_app_send_alert(void){
	int i,id;
	id = 1;
	ESP_LOGE(TAG, "TEST_mqtt_app_send_alert: ENTERING INTO AN INFINITE LOOP!");
	while(1 == 1){
		for(i = 0; i < 6; i++){
			mqtt_app_send_alert("TESTING", id, "Testing alert system every 10 seconds");
			vTaskDelay(100*10);
		}
		id++;
	}
}



