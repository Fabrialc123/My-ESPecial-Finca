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
#include <nvs_app.h>

#include "esp_log.h"
#include "string.h"
#include "mqtt/mqtt_commands.h"
#include "gpios_manager.h"
#include "sensors_manager.h"

// Values related to the SO_SEN sensor in general

static TaskHandle_t task_so_sen 					= NULL;

static const char TAG[] 							= "so_sen";

bool g_so_sen_initialized 							= false;

static pthread_mutex_t mutex_so_sen;

// Arrays for the SO_SEN sensors management

static so_sen_gpios_t* so_sen_gpios_array;
static so_sen_data_t* so_sen_data_array;

static int so_sen_cont								= 0;

// Values related to the alerts

static bool so_sen_alert_soil_moisture				= false;
static int so_sen_soil_moisture_ticks_to_alert		= 5;
static float so_sen_soil_moisture_upper_threshold	= 1000.0;
static float so_sen_soil_moisture_lower_threshold	= -1000.0;

static int* soil_moisture_alert_counter;
static bool* soil_moisture_is_alerted;

// NVS keys

#define nvs_SO_SEN_CONT_key			"so_sen_cont"
#define nvs_SO_SEN_GPIOS_key		"so_sen_gpios"

/**
 * Shift and delete
 */
static void shift_delete(int pos){

	for(int i = pos; i < so_sen_cont - 1; i++){

		so_sen_gpios_array[pos].a0 = so_sen_gpios_array[pos + 1].a0;

		so_sen_data_array[pos].soil_moisture_percentage = so_sen_data_array[pos + 1].soil_moisture_percentage;

		soil_moisture_alert_counter[pos] = soil_moisture_alert_counter[pos + 1];
		soil_moisture_is_alerted[pos] = soil_moisture_is_alerted[pos + 1];
	}

	so_sen_cont--;
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
 * Task for SO_SEN
 */
static void so_sen_task(void *pvParameters){

	int auxRead_raw, read_raw;
	float aux;

	int msg_id = 0;

	for(;;){

		pthread_mutex_lock(&mutex_so_sen);

		for(int i = 0; i < so_sen_cont; i++){

			auxRead_raw = 0;

	        for (int j = 0; j < SO_SEN_N_SAMPLES; j++) {
	        	auxRead_raw += adc1_get_raw(gpio_into_channel(so_sen_gpios_array[i].a0));
	        }

	        read_raw = auxRead_raw / SO_SEN_N_SAMPLES;

	        aux = (((float)SO_SEN_LOW_V - (float)read_raw) / ((float)SO_SEN_LOW_V - (float)SO_SEN_HIGH_V)) * 100;

	        if(aux > 100.0){
	        	so_sen_data_array[i].soil_moisture_percentage = 100.0;
	        }
	        else if(aux < 0.0){
	        	so_sen_data_array[i].soil_moisture_percentage = 0.0;
	        }
	        else{
	        	so_sen_data_array[i].soil_moisture_percentage = aux;
	        }

			// Moisture alert
			if(so_sen_alert_soil_moisture){

				// Update counter
				if((so_sen_data_array[i].soil_moisture_percentage < so_sen_soil_moisture_lower_threshold) && (soil_moisture_alert_counter[i] > -so_sen_soil_moisture_ticks_to_alert))
					soil_moisture_alert_counter[i]--;
				else if((so_sen_data_array[i].soil_moisture_percentage > so_sen_soil_moisture_upper_threshold) && (soil_moisture_alert_counter[i] < so_sen_soil_moisture_ticks_to_alert))
					soil_moisture_alert_counter[i]++;
				else{
					if(soil_moisture_alert_counter[i] > 0)
						soil_moisture_alert_counter[i]--;
					else if(soil_moisture_alert_counter[i] < 0)
						soil_moisture_alert_counter[i]++;
				}

				// Check if the value can be alerted again
				if((soil_moisture_alert_counter[i] == 0) && soil_moisture_is_alerted[i]){
					soil_moisture_is_alerted[i] = false;
				}

				// Check if it is the moment to alert
				if((soil_moisture_alert_counter[i] == -so_sen_soil_moisture_ticks_to_alert) && !soil_moisture_is_alerted[i]){

					mqtt_app_send_alert("SO_SEN", msg_id, "WARNING in Sensor SO_SEN! exceed on lower threshold (value: soil_moisture_percentage)");

					soil_moisture_is_alerted[i] = true;
					msg_id++;
				}
				else if((soil_moisture_alert_counter[i] == so_sen_soil_moisture_ticks_to_alert) && !soil_moisture_is_alerted[i]){

					mqtt_app_send_alert("SO_SEN", msg_id, "WARNING in Sensor SO_SEN! exceed on upper threshold (value: soil_moisture_percentage)");

					soil_moisture_is_alerted[i] = true;
					msg_id++;
				}
			}
		}

        pthread_mutex_unlock(&mutex_so_sen);

        vTaskDelay(SO_SEN_TIME_TO_UPDATE_DATA);
	}
}

void so_sen_startup(void){
	size_t size = 0;
	uint8_t aux = 0;

	nvs_app_get_uint8_value(nvs_SO_SEN_CONT_key,&aux);

	so_sen_cont = aux;

	so_sen_gpios_array = (so_sen_gpios_t*) malloc(sizeof(so_sen_gpios_t) * so_sen_cont);
	so_sen_data_array = (so_sen_data_t*) malloc(sizeof(so_sen_data_t) * so_sen_cont);

	soil_moisture_alert_counter = (int*) malloc(sizeof(int) * so_sen_cont);
	soil_moisture_is_alerted = (bool*) malloc(sizeof(bool) * so_sen_cont);

	adc1_config_width(so_sen_width);

	if(so_sen_cont > 0){
		nvs_app_get_blob_value(nvs_SO_SEN_GPIOS_key,NULL,&size);
		if(size != 0)
			nvs_app_get_blob_value(nvs_SO_SEN_GPIOS_key,so_sen_gpios_array,&size);

		int gpios[SO_SEN_N_GPIOS];
		for(int i = 0; i < so_sen_cont; i++){
			gpios[0] = so_sen_gpios_array[i].a0;
			gpios_manager_lock(gpios, SO_SEN_N_GPIOS);
			adc1_config_channel_atten(gpio_into_channel(gpios[0]), so_sen_atten);
		}

		so_sen_init();
	}
}

void so_sen_init(void){

	if(pthread_mutex_init(&mutex_so_sen, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the SO_SEN mutex");
	}
	else{
		int res_rec, res_sen;

		res_rec = register_recollecter(&so_sen_get_sensors_data, &so_sen_get_sensors_gpios, &so_sen_get_sensors_additional_parameters);
		res_sen = sensors_manager_add(&so_sen_destroy, &so_sen_add_sensor, &so_sen_delete_sensor, &so_sen_set_gpios, &so_sen_set_parameters, &so_sen_set_alert_values);

		if(res_rec == 1 && res_sen == 1){
			ESP_LOGI(TAG, "SO-SEN recollecter successfully registered");

			xTaskCreatePinnedToCore(&so_sen_task, "so_sen_task", SO_SEN_STACK_SIZE, NULL, SO_SEN_PRIORITY, &task_so_sen, SO_SEN_CORE_ID);

			g_so_sen_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, SO-SEN recollecter hasn't been registered");
		}
	}
}

void so_sen_destroy(void){

	if(pthread_mutex_destroy(&mutex_so_sen) != 0){
		ESP_LOGE(TAG,"Failed to destroy the SO-SEN mutex");
	}
	else{
		ESP_LOGI(TAG, "SO-SEN recollecter successfully destroyed");

		vTaskDelete(task_so_sen);

		g_so_sen_initialized = false;
	}
}

int so_sen_add_sensor(int* gpios, union sensor_value_u* parameters){

	int res;

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
		return -1;
	}

	adc1_channel_t channel = gpio_into_channel(gpios[0]);

	if(channel == -1){
		ESP_LOGE(TAG, "Error, the GPIO passed is not an adc1 channel type");
		return -1;
	}

	res = gpios_manager_lock(gpios, SO_SEN_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_so_sen);

		ESP_LOGI(TAG, "Sensor installed");

		adc1_config_channel_atten(channel, so_sen_atten);

		so_sen_cont++;

		so_sen_gpios_array = (so_sen_gpios_t*) realloc(so_sen_gpios_array, sizeof(so_sen_gpios_t) * so_sen_cont);
		so_sen_data_array = (so_sen_data_t*) realloc(so_sen_data_array, sizeof(so_sen_data_t) * so_sen_cont);

		soil_moisture_alert_counter = (int*) realloc(soil_moisture_alert_counter, sizeof(int) * so_sen_cont);
		soil_moisture_is_alerted = (bool*) realloc(soil_moisture_is_alerted, sizeof(bool) * so_sen_cont);

		so_sen_gpios_array[so_sen_cont - 1].a0 = gpios[0];

		so_sen_data_array[so_sen_cont - 1].soil_moisture_percentage = 0.0;

		soil_moisture_alert_counter[so_sen_cont - 1] = 0;
		soil_moisture_is_alerted[so_sen_cont - 1] = false;

		nvs_app_set_uint8_value(nvs_SO_SEN_CONT_key,(uint8_t)so_sen_cont);
		nvs_app_set_blob_value(nvs_SO_SEN_GPIOS_key,so_sen_gpios_array,sizeof(so_sen_gpios_t)*so_sen_cont);

		pthread_mutex_unlock(&mutex_so_sen);
	}

	return res;
}

int so_sen_delete_sensor(int pos){

	int res;

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= so_sen_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	res = gpios_manager_free(&so_sen_gpios_array[pos].a0, SO_SEN_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_so_sen);

		ESP_LOGI(TAG, "Sensor deleted");

		shift_delete(pos);

		so_sen_gpios_array = (so_sen_gpios_t*) realloc(so_sen_gpios_array, sizeof(so_sen_gpios_t) * so_sen_cont);
		so_sen_data_array = (so_sen_data_t*) realloc(so_sen_data_array, sizeof(so_sen_data_t) * so_sen_cont);

		soil_moisture_alert_counter = (int*) realloc(soil_moisture_alert_counter, sizeof(int) * so_sen_cont);
		soil_moisture_is_alerted = (bool*) realloc(soil_moisture_is_alerted, sizeof(bool) * so_sen_cont);

		nvs_app_set_uint8_value(nvs_SO_SEN_CONT_key,(uint8_t)so_sen_cont);
		nvs_app_set_blob_value(nvs_SO_SEN_GPIOS_key,so_sen_gpios_array,sizeof(so_sen_gpios_t)*so_sen_cont);

		pthread_mutex_unlock(&mutex_so_sen);
	}

	return res;
}

int so_sen_set_gpios(int pos, int* gpios){

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= so_sen_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	if(!gpios_manager_is_free(gpios[0])){
		ESP_LOGE(TAG, "Error, the GPIO selected is not available");
		return -1;
	}

	pthread_mutex_lock(&mutex_so_sen);

	gpios_manager_lock(gpios, SO_SEN_N_GPIOS);

	gpios_manager_free(&so_sen_gpios_array[pos].a0, SO_SEN_N_GPIOS);

	so_sen_gpios_array[pos].a0 = gpios[0];

	nvs_app_set_blob_value(nvs_SO_SEN_GPIOS_key,so_sen_gpios_array,sizeof(so_sen_gpios_t)*so_sen_cont);

	pthread_mutex_unlock(&mutex_so_sen);

	return 1;
}

int so_sen_set_parameters(int pos, union sensor_value_u* parameters){
	return -1;
}

int so_sen_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold){

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
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

	pthread_mutex_lock(&mutex_so_sen);

	so_sen_alert_soil_moisture = alert;
	so_sen_soil_moisture_ticks_to_alert = n_ticks;
	so_sen_soil_moisture_upper_threshold = upper_threshold.fval;
	so_sen_soil_moisture_lower_threshold = lower_threshold.fval;

	pthread_mutex_unlock(&mutex_so_sen);

	return 1;
}

sensor_data_t* so_sen_get_sensors_data(int* number_of_sensors){

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
		return NULL;
	}

	if(so_sen_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = so_sen_cont;

	sensor_data_t* aux = (sensor_data_t*) malloc(sizeof(sensor_data_t) * so_sen_cont);
	sensor_value_t *aux2;

	pthread_mutex_lock(&mutex_so_sen);

	for(int i = 0; i < so_sen_cont; i++){

		aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * SO_SEN_N_VALUES);

		strcpy(aux[i].sensorName, "SO-SEN");
		aux[i].valuesLen = SO_SEN_N_VALUES;
		aux[i].sensor_values = aux2;

		aux[i].sensor_values[0].showOnLCD = SO_SEN_SHOW_MOISTURE_PERCENTAGE_ON_LCD;
		strcpy(aux[i].sensor_values[0].valueName,"Soil moisture");
		aux[i].sensor_values[0].sensor_value_type = FLOAT;
		aux[i].sensor_values[0].sensor_value.fval = so_sen_data_array[i].soil_moisture_percentage;
		aux[i].sensor_values[0].alert = so_sen_alert_soil_moisture;
		aux[i].sensor_values[0].ticks_to_alert = so_sen_soil_moisture_ticks_to_alert;
		aux[i].sensor_values[0].upper_threshold.fval = so_sen_soil_moisture_upper_threshold;
		aux[i].sensor_values[0].lower_threshold.fval = so_sen_soil_moisture_lower_threshold;
	}

	pthread_mutex_unlock(&mutex_so_sen);

	return aux;
}

sensor_gpios_info_t* so_sen_get_sensors_gpios(int* number_of_sensors){

	if(!g_so_sen_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the SO_SEN without initializing it");
		return NULL;
	}

	if(so_sen_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = so_sen_cont;

	sensor_gpios_info_t* aux = (sensor_gpios_info_t*) malloc(sizeof(sensor_gpios_info_t) * so_sen_cont);
	sensor_gpio_t *aux2;

	pthread_mutex_lock(&mutex_so_sen);

	for(int i = 0; i < so_sen_cont; i++){

		aux2 = (sensor_gpio_t *)malloc(sizeof(sensor_gpio_t) * SO_SEN_N_GPIOS);

		aux[i].gpiosLen = SO_SEN_N_GPIOS;
		aux[i].sensor_gpios = aux2;

		strcpy(aux[i].sensor_gpios[0].gpioName,"A0");
		aux[i].sensor_gpios[0].sensor_gpio = so_sen_gpios_array[i].a0;
	}

	pthread_mutex_unlock(&mutex_so_sen);

	return aux;
}

sensor_additional_parameters_info_t* so_sen_get_sensors_additional_parameters(int* number_of_sensors){
	return NULL;
}
