/*
 * tests.c
 *
 *  Created on: 26 sept. 2022
 *      Author: fabri
 */

#include "mqtt_app.c"

mqtt_app_sensor_data_t mqtt_app_recollecter_test (void){
	mqtt_app_sensor_data_t aux;

	strcpy(aux.sensorName, "TESTING");
	aux.sensorData = 1111;

	return aux;
}

void mqtt_app_recollect_test (void){
	int i;
	for(i = 0; i < MQTT_APP_SENSOR_DATA_SIZE; i++ ){
	 mqtt_app_register_recollector(&mqtt_app_recollecter_test);
	}
	 while(1){
		 vTaskDelay(100);
		 mqtt_app_send_message(MQTT_APP_MSG_SEND_DATA);
	 }
}




