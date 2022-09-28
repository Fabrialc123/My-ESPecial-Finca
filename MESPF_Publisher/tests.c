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

sensor_data_t TEST_recollecter (void){
	sensor_data_t aux;
	sensor_value_t *aux2;

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t));

	strcpy(aux.sensorName, "TESTING");
	aux.valuesLen = 1;
	aux.sensor_values = aux2;
	aux.sensor_values[0].sensor_value_type = INTEGER;
	strcpy(aux.sensor_values[0].valueName,"NAMETEST");
	aux.sensor_values[0].sensor_value.ival = 1111;

	return aux;
}

void TEST_mqtt_app_recollect(void){
	int i;
	int aux;
/*	int TEST_SEND_DELAY = 100; // 1 second every 100 ticks +/-*/
	for (i = 0; i < RECOLLECTER_SIZE;i++){
	 aux = register_recollecter(&TEST_recollecter);
	 ESP_LOGE(TAG, "registered %d recollecter with result = %d", i,aux);
	}

/*	 while(1){
		 mqtt_app_send_message(MQTT_APP_MSG_SEND_DATA);
		 vTaskDelay(TEST_SEND_DELAY);
	 }*/
}




