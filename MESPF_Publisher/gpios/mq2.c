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
#include <nvs_app.h>

#include "esp_log.h"
#include "string.h"
#include "mqtt/mqtt_commands.h"
#include "gpios_manager.h"
#include "sensors_manager.h"

// Values related to the MQ2 sensor in general

static TaskHandle_t task_mq2 				= NULL;

static const char TAG[] 					= "mq2";

bool g_mq2_initialized 						= false;

static pthread_mutex_t mutex_mq2;

// Arrays for the MQ2 sensors management

static mq2_gpios_t* mq2_gpios_array;
static mq2_data_t* mq2_data_array;

static int mq2_cont							= 0;

// Values related to the alerts

static bool mq2_alert_smoke_gas				= false;
static int mq2_smoke_gas_ticks_to_alert		= 5;
static float mq2_smoke_gas_upper_threshold	= 1000.0;
static float mq2_smoke_gas_lower_threshold	= -1000.0;

static int* smoke_gas_alert_counter;
static bool* smoke_gas_is_alerted;

// NVS keys

#define nvs_MQ2_CONT_key			"mq2_cont"
#define nvs_MQ2_GPIOS_key			"mq2_gpios"

/**
 * Shift and delete
 */
static void shift_delete(int pos){

	for(int i = pos; i < mq2_cont - 1; i++){

		mq2_gpios_array[pos].a0 = mq2_gpios_array[pos + 1].a0;

		mq2_data_array[pos].smoke_gas_percentage = mq2_data_array[pos + 1].smoke_gas_percentage;

		smoke_gas_alert_counter[pos] = smoke_gas_alert_counter[pos + 1];
		smoke_gas_is_alerted[pos] = smoke_gas_is_alerted[pos + 1];
	}

	mq2_cont--;
}

/**
 * Convert GPIO into channel
 */

static adc1_channel_t gpio_into_channel(int gpio){

	adc1_channel_t channel;

	switch (gpio) {
		case 36:
			channel = ADC1_CHANNEL_0;
			break;
		case 37:
			channel = ADC1_CHANNEL_1;
			break;
		case 38:
			channel = ADC1_CHANNEL_2;
			break;
		case 39:
			channel = ADC1_CHANNEL_3;
			break;
		case 32:
			channel = ADC1_CHANNEL_4;
			break;
		case 33:
			channel = ADC1_CHANNEL_5;
			break;
		case 34:
			channel = ADC1_CHANNEL_6;
			break;
		case 35:
			channel = ADC1_CHANNEL_7;
			break;
		default:
			channel = -1;
			break;
	}

	return channel;
}

/**
 * Task for MQ2
 */
static void mq2_task(void *pvParameters){

	int auxRead_raw, read_raw;
	float aux;

	int msg_id = 0;

	for(;;){

		pthread_mutex_lock(&mutex_mq2);

		for(int i = 0; i < mq2_cont; i++){

			auxRead_raw = 0;

	        for (int j = 0; j < MQ2_N_SAMPLES; j++) {
	        	auxRead_raw += adc1_get_raw(gpio_into_channel(mq2_gpios_array[i].a0));
	        }

	        read_raw = auxRead_raw / MQ2_N_SAMPLES;

	        aux = (((float)read_raw - (float)MQ2_LOW_V) / ((float)MQ2_HIGH_V - (float)MQ2_LOW_V)) * 100;

	        if(aux > 100.0){
	        	mq2_data_array[i].smoke_gas_percentage = 100.0;
	        }
	        else if(aux < 0.0){
	        	mq2_data_array[i].smoke_gas_percentage = 0.0;
	        }
	        else{
	        	mq2_data_array[i].smoke_gas_percentage = aux;
	        }

			// Smoke gas alert
			if(mq2_alert_smoke_gas){

				// Update counter
				if((mq2_data_array[i].smoke_gas_percentage < mq2_smoke_gas_lower_threshold) && (smoke_gas_alert_counter[i] > -mq2_smoke_gas_ticks_to_alert))
					smoke_gas_alert_counter[i]--;
				else if((mq2_data_array[i].smoke_gas_percentage > mq2_smoke_gas_upper_threshold) && (smoke_gas_alert_counter[i] < mq2_smoke_gas_ticks_to_alert))
					smoke_gas_alert_counter[i]++;
				else{
					if(smoke_gas_alert_counter[i] > 0)
						smoke_gas_alert_counter[i]--;
					else if(smoke_gas_alert_counter[i] < 0)
						smoke_gas_alert_counter[i]++;
				}

				// Check if the value can be alerted again
				if((smoke_gas_alert_counter[i] == 0) && smoke_gas_is_alerted[i]){
					smoke_gas_is_alerted[i] = false;
				}

				// Check if it is the moment to alert
				if((smoke_gas_alert_counter[i] == -mq2_smoke_gas_ticks_to_alert) && !smoke_gas_is_alerted[i]){

					mqtt_app_send_alert("MQ2", msg_id, "WARNING in Sensor MQ2! exceed on lower threshold (value: smoke_gas_percentage)");

					smoke_gas_is_alerted[i] = true;
					msg_id++;
				}
				else if((smoke_gas_alert_counter[i] == mq2_smoke_gas_ticks_to_alert) && !smoke_gas_is_alerted[i]){

					mqtt_app_send_alert("MQ2", msg_id, "WARNING in Sensor MQ2! exceed on upper threshold (value: smoke_gas_percentage)");

					smoke_gas_is_alerted[i] = true;
					msg_id++;
				}
			}
		}

        pthread_mutex_unlock(&mutex_mq2);

        vTaskDelay(MQ2_TIME_TO_UPDATE_DATA);
	}
}

void mq2_startup(void){
	size_t size = 0;
	uint8_t aux = 0;

	nvs_app_get_uint8_value(nvs_MQ2_CONT_key,&aux);

	mq2_cont = aux;

	mq2_gpios_array = (mq2_gpios_t*) malloc(sizeof(mq2_gpios_t) * mq2_cont);
	mq2_data_array = (mq2_data_t*) malloc(sizeof(mq2_data_t) * mq2_cont);

	smoke_gas_alert_counter = (int*) malloc(sizeof(int) * mq2_cont);
	smoke_gas_is_alerted = (bool*) malloc(sizeof(bool) * mq2_cont);

	adc1_config_width(mq2_width);

	if(mq2_cont > 0){
		nvs_app_get_blob_value(nvs_MQ2_GPIOS_key,NULL,&size);
		if(size != 0)
			nvs_app_get_blob_value(nvs_MQ2_GPIOS_key,mq2_gpios_array,&size);

		int gpios[MQ2_N_GPIOS];
		for(int i = 0; i < mq2_cont; i++){
			gpios[0] = mq2_gpios_array[i].a0;
			gpios_manager_lock(gpios, MQ2_N_GPIOS);
			adc1_config_channel_atten(gpio_into_channel(gpios[0]), mq2_atten);
		}

		mq2_init();
	}
}

void mq2_init(void){

	if(pthread_mutex_init(&mutex_mq2, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the MQ2 mutex");
	}
	else{
		int res_rec, res_sen;

		res_rec = register_recollecter(&mq2_get_sensors_data, &mq2_get_sensors_gpios, &mq2_get_sensors_additional_parameters);
		res_sen = sensors_manager_add(&mq2_destroy, &mq2_add_sensor, &mq2_delete_sensor, &mq2_set_gpios, &mq2_set_parameters, &mq2_set_alert_values);

		if(res_rec == 1 && res_sen == 1){
			ESP_LOGI(TAG, "MQ2 recollecter successfully registered");

			xTaskCreatePinnedToCore(&mq2_task, "mq2_task", MQ2_STACK_SIZE, NULL, MQ2_PRIORITY, &task_mq2, MQ2_CORE_ID);

			g_mq2_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, MQ2 recollecter or sensor manager hasn't been registered");
		}
	}
}

void mq2_destroy(void){

	if(pthread_mutex_destroy(&mutex_mq2) != 0){
		ESP_LOGE(TAG,"Failed to destroy the MQ2 mutex");
	}
	else{
		ESP_LOGI(TAG, "MQ2 recollecter and sensor manager successfully destroyed");

		vTaskDelete(task_mq2);

		g_mq2_initialized = false;
	}
}

int mq2_add_sensor(int* gpios, union sensor_value_u* parameters){

	int res;

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		return -1;
	}

	adc1_channel_t channel = gpio_into_channel(gpios[0]);

	if(channel == -1){
		ESP_LOGE(TAG, "Error, the GPIO passed is not an adc1 channel type");
		return -1;
	}

	res = gpios_manager_lock(gpios, MQ2_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_mq2);

		ESP_LOGI(TAG, "Sensor installed");

		adc1_config_channel_atten(channel, mq2_atten);

		mq2_cont++;

		mq2_gpios_array = (mq2_gpios_t*) realloc(mq2_gpios_array, sizeof(mq2_gpios_t) * mq2_cont);
		mq2_data_array = (mq2_data_t*) realloc(mq2_data_array, sizeof(mq2_data_t) * mq2_cont);

		smoke_gas_alert_counter = (int*) realloc(smoke_gas_alert_counter, sizeof(int) * mq2_cont);
		smoke_gas_is_alerted = (bool*) realloc(smoke_gas_is_alerted, sizeof(bool) * mq2_cont);

		mq2_gpios_array[mq2_cont - 1].a0 = gpios[0];

		mq2_data_array[mq2_cont - 1].smoke_gas_percentage = 0.0;

		smoke_gas_alert_counter[mq2_cont - 1] = 0;
		smoke_gas_is_alerted[mq2_cont - 1] = false;

		nvs_app_set_uint8_value(nvs_MQ2_CONT_key,(uint8_t)mq2_cont);
		nvs_app_set_blob_value(nvs_MQ2_GPIOS_key,mq2_gpios_array,sizeof(mq2_gpios_t)*mq2_cont);

		pthread_mutex_unlock(&mutex_mq2);
	}

	return res;
}

int mq2_delete_sensor(int pos){

	int res;

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_mq2);

	if(pos < 0 || pos >= mq2_cont){
		pthread_mutex_unlock(&mutex_mq2);

		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	res = gpios_manager_free(&mq2_gpios_array[pos].a0, MQ2_N_GPIOS);

	if(res == 1){

		ESP_LOGI(TAG, "Sensor deleted");

		shift_delete(pos);

		mq2_gpios_array = (mq2_gpios_t*) realloc(mq2_gpios_array, sizeof(mq2_gpios_t) * mq2_cont);
		mq2_data_array = (mq2_data_t*) realloc(mq2_data_array, sizeof(mq2_data_t) * mq2_cont);

		smoke_gas_alert_counter = (int*) realloc(smoke_gas_alert_counter, sizeof(int) * mq2_cont);
		smoke_gas_is_alerted = (bool*) realloc(smoke_gas_is_alerted, sizeof(bool) * mq2_cont);

		nvs_app_set_uint8_value(nvs_MQ2_CONT_key,(uint8_t)mq2_cont);
		nvs_app_set_blob_value(nvs_MQ2_GPIOS_key,mq2_gpios_array,sizeof(mq2_gpios_t)*mq2_cont);
	}

	pthread_mutex_unlock(&mutex_mq2);

	return res;
}

int mq2_set_gpios(int pos, int* gpios){

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_mq2);

	if(pos < 0 || pos >= mq2_cont){
		pthread_mutex_unlock(&mutex_mq2);

		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	if(!gpios_manager_is_free(gpios[0])){
		pthread_mutex_unlock(&mutex_mq2);

		ESP_LOGE(TAG, "Error, the GPIO selected is not available");
		return -1;
	}

	gpios_manager_lock(gpios, MQ2_N_GPIOS);

	gpios_manager_free(&mq2_gpios_array[pos].a0, MQ2_N_GPIOS);

	mq2_gpios_array[pos].a0 = gpios[0];

	nvs_app_set_blob_value(nvs_MQ2_GPIOS_key,mq2_gpios_array,sizeof(mq2_gpios_t)*mq2_cont);

	pthread_mutex_unlock(&mutex_mq2);

	return 1;
}

int mq2_set_parameters(int pos, union sensor_value_u* parameters){
	return -1;
}

int mq2_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold){

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		return -1;
	}

	if(value < 0 || value > 0){
		ESP_LOGE(TAG, "Error, the value doesn't exist");
		return -1;
	}

	if(n_ticks <= 0){
		ESP_LOGE(TAG, "Error, the number of ticks can't be 0 or less than 0");
		return -1;
	}

	if(upper_threshold.fval <= lower_threshold.fval){
		ESP_LOGE(TAG, "Error, the thresholds don't make sense, upper is smaller than lower");
		return -1;
	}

	pthread_mutex_lock(&mutex_mq2);

	mq2_alert_smoke_gas = alert;
	mq2_smoke_gas_ticks_to_alert = n_ticks;
	mq2_smoke_gas_upper_threshold = upper_threshold.fval;
	mq2_smoke_gas_lower_threshold = lower_threshold.fval;

	pthread_mutex_unlock(&mutex_mq2);

	return 1;
}

sensor_data_t* mq2_get_sensors_data(int* number_of_sensors){

	*number_of_sensors = 0;

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		return NULL;
	}

	pthread_mutex_lock(&mutex_mq2);

	if(mq2_cont == 0){
		pthread_mutex_unlock(&mutex_mq2);

		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = mq2_cont;

	sensor_data_t* aux = (sensor_data_t*) malloc(sizeof(sensor_data_t) * mq2_cont);
	sensor_value_t *aux2;

	for(int i = 0; i < mq2_cont; i++){

		aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * MQ2_N_VALUES);

		strcpy(aux[i].sensorName, "MQ2");
		aux[i].valuesLen = MQ2_N_VALUES;
		aux[i].sensor_values = aux2;

		aux[i].sensor_values[0].showOnLCD = MQ2_SHOW_SMOKE_GAS_PERCENTAGE_ON_LCD;
		strcpy(aux[i].sensor_values[0].valueName,"S/G");
		aux[i].sensor_values[0].sensor_value_type = FLOAT;
		aux[i].sensor_values[0].sensor_value.fval = mq2_data_array[i].smoke_gas_percentage;
		aux[i].sensor_values[0].alert = mq2_alert_smoke_gas;
		aux[i].sensor_values[0].ticks_to_alert = mq2_smoke_gas_ticks_to_alert;
		aux[i].sensor_values[0].upper_threshold.fval = mq2_smoke_gas_upper_threshold;
		aux[i].sensor_values[0].lower_threshold.fval = mq2_smoke_gas_lower_threshold;
	}

	pthread_mutex_unlock(&mutex_mq2);

	return aux;
}

sensor_gpios_info_t* mq2_get_sensors_gpios(int* number_of_sensors){

	*number_of_sensors = 0;

	if(!g_mq2_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing it");
		return NULL;
	}

	pthread_mutex_lock(&mutex_mq2);

	if(mq2_cont == 0){
		pthread_mutex_unlock(&mutex_mq2);

		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = mq2_cont;

	sensor_gpios_info_t* aux = (sensor_gpios_info_t*) malloc(sizeof(sensor_gpios_info_t) * mq2_cont);
	sensor_gpio_t *aux2;

	for(int i = 0; i < mq2_cont; i++){

		aux2 = (sensor_gpio_t *)malloc(sizeof(sensor_gpio_t) * MQ2_N_GPIOS);

		aux[i].gpiosLen = MQ2_N_GPIOS;
		aux[i].sensor_gpios = aux2;

		strcpy(aux[i].sensor_gpios[0].gpioName,"A0");
		aux[i].sensor_gpios[0].sensor_gpio = mq2_gpios_array[i].a0;
	}

	pthread_mutex_unlock(&mutex_mq2);

	return aux;
}

sensor_additional_parameters_info_t* mq2_get_sensors_additional_parameters(int* number_of_sensors){
	return NULL;
}
