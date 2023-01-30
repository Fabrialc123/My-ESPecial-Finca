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
#include "mqtt/mqtt_commands.h"

static const char TAG[] = "mq2";

bool g_mq2_initialized 	= false;

static float smoke_gas_percentage;

static pthread_mutex_t mutex_mq2;

/**
 * Task for MQ2
 */

static void mq2_task(void *pvParameters){

	int auxRead_raw, read_raw;

	// Variables related to the alerts

	int msg_id = 0;

	int smoke_gas_percentage_alert_counter = 0;

	bool smoke_gas_percentage_is_alerted = false;

	for(;;){

		pthread_mutex_lock(&mutex_mq2);

		auxRead_raw = 0;

        for (int i = 0; i < N_SAMPLES; i++) {
        	auxRead_raw += adc1_get_raw(channel_mq2);
        }

        read_raw = auxRead_raw / N_SAMPLES;

        smoke_gas_percentage = (((float)read_raw - (float)MQ2_LOW_V) / ((float)MQ2_HIGH_V - (float)MQ2_LOW_V)) * 100;

		// Smoke gas alert
		if(MQ2_ALERT_SMOKE_GAS){

			// Update counter
			if((smoke_gas_percentage < MQ2_SMOKE_GAS_LOWER_THRESHOLD) && (smoke_gas_percentage_alert_counter > -MQ2_SMOKE_GAS_TICKS_TO_ALERT))
				smoke_gas_percentage_alert_counter--;
			else if((smoke_gas_percentage > MQ2_SMOKE_GAS_UPPER_THRESHOLD) && (smoke_gas_percentage_alert_counter < MQ2_SMOKE_GAS_TICKS_TO_ALERT))
				smoke_gas_percentage_alert_counter++;
			else{
				if(smoke_gas_percentage_alert_counter > 0)
					smoke_gas_percentage_alert_counter--;
				else if(smoke_gas_percentage_alert_counter < 0)
					smoke_gas_percentage_alert_counter++;
			}

			// Check if the value can be alerted again
			if((smoke_gas_percentage_alert_counter == 0) && smoke_gas_percentage_is_alerted){
				smoke_gas_percentage_is_alerted = false;
			}

			// Check if it is the moment to alert
			if((smoke_gas_percentage_alert_counter == -MQ2_SMOKE_GAS_TICKS_TO_ALERT) && !smoke_gas_percentage_is_alerted){

				mqtt_app_send_alert("MQ2", msg_id, "WARNING in Sensor MQ2! exceed on lower threshold (value: smoke_gas_percentage)");

				smoke_gas_percentage_is_alerted = true;
				msg_id++;
			}
			else if((smoke_gas_percentage_alert_counter == MQ2_SMOKE_GAS_TICKS_TO_ALERT) && !smoke_gas_percentage_is_alerted){

				mqtt_app_send_alert("MQ2", msg_id, "WARNING in Sensor MQ2! exceed on upper threshold (value: smoke_gas_percentage)");

				smoke_gas_percentage_is_alerted = true;
				msg_id++;
			}
		}

        pthread_mutex_unlock(&mutex_mq2);

        vTaskDelay(MQ2_TIME_TO_UPDATE_DATA);
	}
}

void mq2_init(void){

	if(pthread_mutex_init(&mutex_mq2, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the MQ2 mutex");
	}
	else{
		int res;

		res = register_recollecter(&mq2_get_sensor_data);

		if(res == 1){
			ESP_LOGI(TAG, "MQ2 recollecter successfully registered");

			adc1_config_width(width_mq2);
			adc1_config_channel_atten(channel_mq2, atten_mq2);

			smoke_gas_percentage = 0.0;

			xTaskCreatePinnedToCore(&mq2_task, "mq2_task", MQ2_STACK_SIZE, NULL, MQ2_PRIORITY, NULL, MQ2_CORE_ID);

			g_mq2_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, MQ2 recollecter hasn't been registered");
		}
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

	aux.sensor_values[0].showOnLCD = MQ2_SHOW_SMOKE_GAS_PERCENTAGE_ON_LCD;
	strcpy(aux.sensor_values[0].valueName,"S/G");
	aux.sensor_values[0].sensor_value_type = FLOAT;
	aux.sensor_values[0].sensor_value.fval = smoke_gas_percentage;
	aux.sensor_values[0].upper_threshold.fval = MQ2_SMOKE_GAS_UPPER_THRESHOLD;
	aux.sensor_values[0].lower_threshold.fval = MQ2_SMOKE_GAS_LOWER_THRESHOLD;

	pthread_mutex_unlock(&mutex_mq2);

	return aux;
}
