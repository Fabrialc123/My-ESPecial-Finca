/*
 * recollecter.c
 *
 *  Created on: 27 sept. 2022
 *      Author: fabri
 */

#include "esp_log.h"
#include "string.h"

#include "recollecter.h"
#include <pthread.h>

static pthread_mutex_t mutex_RECOLLECTER;
static recollecter_function *recollecters;
static int recollecters_n;


static const char TAG[] = "RECOLLECTER";

int register_recollecter (recollecter_function rtr){
	int res = 0;

pthread_mutex_lock(&mutex_RECOLLECTER);
	 if (recollecters_n >= RECOLLECTER_SIZE){
		res = -1;
		ESP_LOGE(TAG, "ERROR in register_recollecter, the array is FULL (%d/%d)! ",recollecters_n, RECOLLECTER_SIZE);
	}
	else {
		recollecters[recollecters_n++] = rtr;
		res = 1;
	}
pthread_mutex_unlock(&mutex_RECOLLECTER);

	return res;
}

int get_recollecters_size (void){
	int size = 0;

pthread_mutex_lock(&mutex_RECOLLECTER);
	size = recollecters_n;
pthread_mutex_unlock(&mutex_RECOLLECTER);

	return size;
}

int get_sensor_data (int sensor_id, char *data){
	char aux[CHAR_LENGTH];
	recollecter_function foo;
	sensor_data_t sensor_data;
	int i;
	int len = 0;


	pthread_mutex_lock(&mutex_RECOLLECTER);
		if (sensor_id >= recollecters_n){
			len = -1;
			ESP_LOGE(TAG, "ERROR in get_sensor_data, the sensor_id is out of range!");
		}else {
			foo = recollecters[sensor_id];
		}
	pthread_mutex_unlock(&mutex_RECOLLECTER);

	if (len == -1) return -1;

	sensor_data = foo();

	strcat(data, "{\"");
	strcat(data, sensor_data.sensorName);
	strcat(data, "\":");
	strcat(data, "{");
	len += 5 + strlen(sensor_data.sensorName);
	for (i = 0; i < sensor_data.valuesLen;i++){
		if (i > 0){
			strcat(data,",");
			len++;
		}
		strcat(data,"\"");
		strcat(data, sensor_data.sensor_values[i].valueName);
		strcat(data,"\":");
		if (sensor_data.sensor_values[i].sensor_value_type == INTEGER){
			sprintf(aux, "%d",sensor_data.sensor_values[i].sensor_value.ival);
		}else if (sensor_data.sensor_values[i].sensor_value_type == FLOAT){
			sprintf(aux, "%f",sensor_data.sensor_values[i].sensor_value.fval);
		}else {
			strcpy(aux, "\"");
			strcat(aux,sensor_data.sensor_values[i].sensor_value.cval );
			strcat(aux, "\"");
		}
		strcat(data, aux);


		len += 1 + strlen( sensor_data.sensor_values[i].valueName) + 1 + strlen(aux) + 1;
	}
	strcat(data, "}");
	strcat(data, "}");
	len += 2;

	return len;
}

void recollecter_start(void){
	if(pthread_mutex_init (&mutex_RECOLLECTER, NULL) != 0){
	 ESP_LOGE(TAG,"Failed to initialize the recollecter mutex");
	}

    recollecters = (recollecter_function *)malloc(sizeof(recollecter_function) * RECOLLECTER_SIZE);
    recollecters_n = 0;

    ESP_LOGI(TAG, "recollecter started");
}




