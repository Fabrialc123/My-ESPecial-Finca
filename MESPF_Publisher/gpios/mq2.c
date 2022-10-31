/*
 * mq2.c
 *
 *  Created on: 11 oct. 2022
 *      Author: Kike
 */

#include "gpios/mq2.h"
#include "task_common.h"

#include <stdbool.h>
#include <pthread.h>

#include "esp_log.h"
#include "string.h"

static const char TAG[] = "mq2";

bool g_mq2_initialized 	= false;

static int read_raw;

static pthread_mutex_t mutex_mq2;

/**
 * Task for MQ2
 */

static void mq2_task(void *pvParameters){

	int auxRead_raw;

	for(;;){

		pthread_mutex_lock(&mutex_mq2);

		auxRead_raw = 0;

        for (int i = 0; i < N_SAMPLES; i++) {
        	auxRead_raw += adc1_get_raw(channel);
        }

        read_raw = auxRead_raw / N_SAMPLES;

        pthread_mutex_unlock(&mutex_mq2);

        vTaskDelay(MQ2_TIME_TO_UPDATE_DATA);
	}
}

void mq2_init(void){

	if(pthread_mutex_init(&mutex_mq2, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the MQ2 mutex");
	}
	else{

		adc1_config_width(width);
		adc1_config_channel_atten(channel, atten);

		read_raw = 0;

		xTaskCreatePinnedToCore(&mq2_task, "mq2_task", MQ2_STACK_SIZE, NULL, MQ2_PRIORITY, NULL, MQ2_CORE_ID);

		g_mq2_initialized = true;
	}
}

sensor_data_t mq2_get_sensor_data(void){

	sensor_data_t aux;
	sensor_value_t *aux2;
	int number_of_values = 1;

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		aux.valuesLen = 0;
		return aux;
	}

	pthread_mutex_lock(&mutex_mq2);

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "MQ2");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;

	strcpy(aux.sensor_values[0].valueName,"S/G");
	aux.sensor_values[0].sensor_value_type = INTEGER;
	aux.sensor_values[0].sensor_value.ival = read_raw;

	pthread_mutex_unlock(&mutex_mq2);

	return aux;
}
