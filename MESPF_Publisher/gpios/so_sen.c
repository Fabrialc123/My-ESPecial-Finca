/*
 * so_sen.c
 *
 *  Created on: 4 nov. 2022
 *      Author: Kike
 */

#include "gpios/so_sen.h"
#include "task_common.h"

#include <stdbool.h>
#include <pthread.h>

#include "esp_log.h"
#include "string.h"

static const char TAG[] = "so_sen";

bool g_so_sen_initialized = false;

static int read_raw;

static pthread_mutex_t mutex_so_sen;

/**
 * Task for SO_SEN
 */

static void so_sen_task(void *pvParameters){

	int auxRead_raw;

	for(;;){

		pthread_mutex_lock(&mutex_so_sen);

		auxRead_raw = 0;

        for (int i = 0; i < N_SAMPLES; i++) {
        	auxRead_raw += adc1_get_raw(channel_so_sen);
        }

        read_raw = auxRead_raw / N_SAMPLES;

        pthread_mutex_unlock(&mutex_so_sen);

        vTaskDelay(SO_SEN_TIME_TO_UPDATE_DATA);
	}
}

void so_sen_init(void){

	if(pthread_mutex_init(&mutex_so_sen, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the SO_SEN mutex");
	}
	else{
		int res;

		res = register_recollecter(&so_sen_get_sensor_data);

		if(res == 1){
			ESP_LOGI(TAG, "SO-SEN recollecter successfully registered");

			adc1_config_width(width_so_sen);
			adc1_config_channel_atten(channel_so_sen, atten_so_sen);

			read_raw = 0;

			xTaskCreatePinnedToCore(&so_sen_task, "so_sen_task", SO_SEN_STACK_SIZE, NULL, SO_SEN_PRIORITY, NULL, SO_SEN_CORE_ID);

			g_so_sen_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, SO-SEN recollecter hasn't been registered");
		}
	}
}

sensor_data_t so_sen_get_sensor_data(void){

	sensor_data_t aux;
	sensor_value_t *aux2;
	int number_of_values = 1;

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
		aux.valuesLen = 0;
		return aux;
	}

	pthread_mutex_lock(&mutex_so_sen);

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "SO-SEN");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;

	strcpy(aux.sensor_values[0].valueName,"Soil moisture");
	aux.sensor_values[0].sensor_value_type = INTEGER;
	aux.sensor_values[0].sensor_value.ival = read_raw;
	
	aux.showOnLCD = SO_SEN_SHOW_ON_LCD;

	pthread_mutex_unlock(&mutex_so_sen);

	return aux;
}
