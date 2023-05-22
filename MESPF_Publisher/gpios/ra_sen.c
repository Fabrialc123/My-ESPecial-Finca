/*
 * ra_sen.c
 *
 *  Created on: 22 may. 2023
 *      Author: Kike
 */

#include "gpios/ra_sen.h"
#include "task_common.h"

#include <stdbool.h>
#include <pthread.h>
#include <nvs_app.h>

#include "esp_log.h"
#include "string.h"
#include "mqtt/mqtt_commands.h"
#include "gpios_manager.h"
#include "sensors_manager.h"

// RA_SEN general information

static TaskHandle_t task_ra_sen 					= NULL;

static const char TAG[] 							= "ra_sen";

bool g_ra_sen_initialized 							= false;

static pthread_mutex_t mutex_ra_sen;

// Arrays for the RA_SEN sensors management

static ra_sen_gpios_t* ra_sen_gpios_array;
static ra_sen_data_t* ra_sen_data_array;
static char** ra_sen_locations_array;

static int ra_sen_cont								= 0;

// Values related to the alerts

static ra_sen_alerts_t ra_sen_alerts;

	/* Wet */
static int* wet_alert_counter;
static int* wet_last_alert_counter;

// NVS keys

#define nvs_RA_SEN_CONT_key			"ra_sen_cont"
#define nvs_RA_SEN_GPIOS_key		"ra_sen_gpios"
#define nvs_RA_SEN_ALERTS_key		"ra_sen_alert"
#define nvs_RA_SEN_LOCATIONS_key	"ra_sen_loc"

/**
 * Check if position is valid
 */
static bool check_valid_pos(int pos){
	bool valid;

	pthread_mutex_lock(&mutex_ra_sen);

	if(pos < 0 || pos >= ra_sen_cont)
		valid = false;
	else
		valid = true;

	pthread_mutex_unlock(&mutex_ra_sen);

	return valid;
}

/**
 * Check if GPIOS can be liberated
 */
static bool check_free_gpios(int pos, char* reason){
	bool valid;

	pthread_mutex_lock(&mutex_ra_sen);

	if(gpios_manager_free_gpios(&ra_sen_gpios_array[pos].a0, RA_SEN_N_GPIOS, reason) == -1)
		valid = false;
	else
		valid = true;

	pthread_mutex_unlock(&mutex_ra_sen);

	return valid;
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
 * Task for RA_SEN
 */
static void ra_sen_task(void *pvParameters){

	int auxRead_raw, read_raw;
	float aux;

	int msg_id = 0;

	for(;;){

		pthread_mutex_lock(&mutex_ra_sen);

		for(int i = 0; i < ra_sen_cont; i++){

			auxRead_raw = 0;

	        for (int j = 0; j < RA_SEN_N_SAMPLES; j++) {
	        	auxRead_raw += adc1_get_raw(gpio_into_channel(ra_sen_gpios_array[i].a0));
	        }

	        read_raw = auxRead_raw / RA_SEN_N_SAMPLES;

	        aux = (((float)RA_SEN_LOW_V - (float)read_raw) / ((float)RA_SEN_LOW_V - (float)RA_SEN_HIGH_V)) * 100;

	        if(aux > 100.0){
	        	ra_sen_data_array[i].wet_percentage = 100.0;
	        }
	        else if(aux < 0.0){
	        	ra_sen_data_array[i].wet_percentage = 0.0;
	        }
	        else{
	        	ra_sen_data_array[i].wet_percentage = aux;
	        }

			// Wet alert
			if(ra_sen_alerts.wet_alert){

				// Update counter
				if(ra_sen_data_array[i].wet_percentage < ra_sen_alerts.wet_lower_threshold)
					wet_alert_counter[i]--;
				else if(ra_sen_data_array[i].wet_percentage > ra_sen_alerts.wet_upper_threshold)
					wet_alert_counter[i]++;
				else{
					if(wet_alert_counter[i] > 0)
						wet_alert_counter[i]--;
					else if(wet_alert_counter[i] < 0)
						wet_alert_counter[i]++;
				}

				// Update last alert counter
				if(wet_last_alert_counter[i] < RA_SEN_STABILIZED_TICKS)
					wet_last_alert_counter[i]++;

				// Check if it is the moment to alert
				if(wet_alert_counter[i] <= -ra_sen_alerts.wet_ticks_to_alert){

					if(wet_last_alert_counter[i] == RA_SEN_STABILIZED_TICKS)
						msg_id++;

					mqtt_app_send_alert("RA_SEN", i, msg_id, "WARNING! exceed on lower threshold (value: wet_percentage)");

					wet_alert_counter[i] = 0;
					wet_last_alert_counter[i] = 0;
				}
				else if(wet_alert_counter[i] >= ra_sen_alerts.wet_ticks_to_alert){

					if(wet_last_alert_counter[i] == RA_SEN_STABILIZED_TICKS)
						msg_id++;

					mqtt_app_send_alert("RA_SEN", i, msg_id, "WARNING! exceed on upper threshold (value: wet_percentage)");

					wet_alert_counter[i] = 0;
					wet_last_alert_counter[i] = 0;
				}
			}
		}


        pthread_mutex_unlock(&mutex_ra_sen);

        vTaskDelay(RA_SEN_TIME_TO_UPDATE_DATA);
	}
}

void ra_sen_init(void){

	if(g_ra_sen_initialized == true)
		ESP_LOGE(TAG,"RA-SEN already initialized");
	else if(pthread_mutex_init(&mutex_ra_sen, NULL) != 0)
		ESP_LOGE(TAG,"Failed to initialize the RA-SEN mutex");
	else{
		int res_rec, res_sen;

		res_rec = register_recollecter(&ra_sen_get_sensors_data, &ra_sen_get_sensors_gpios, &ra_sen_get_sensors_additional_parameters);
		res_sen = sensors_manager_add(&ra_sen_add_sensor, &ra_sen_delete_sensor, &ra_sen_set_gpios, &ra_sen_set_parameters, &ra_sen_set_location, &ra_sen_set_alert_values);

		if(res_rec == 1 && res_sen == 1){

			size_t size = 0;
			uint8_t aux = 0;

			/* Load & Startup information */

			// Get "cont"
			nvs_app_get_uint8_value(nvs_RA_SEN_CONT_key,&aux);
			ra_sen_cont = aux;

			// Initialize pointers
			ra_sen_gpios_array = (ra_sen_gpios_t*) malloc(sizeof(ra_sen_gpios_t) * ra_sen_cont);
			memset(ra_sen_gpios_array,0,sizeof(ra_sen_gpios_t) * ra_sen_cont);
			ra_sen_data_array = (ra_sen_data_t*) malloc(sizeof(ra_sen_data_t) * ra_sen_cont);
			memset(ra_sen_data_array,0,sizeof(ra_sen_data_t) * ra_sen_cont);
			ra_sen_locations_array = (char**) malloc(sizeof(char*) * ra_sen_cont);

				// Initialize locations
			for(int i = 0; i < ra_sen_cont; i++){
				ra_sen_locations_array[i] = (char*) malloc(CHAR_LENGTH + 1);
				memset(ra_sen_locations_array[i],0,CHAR_LENGTH + 1);
			}

			wet_alert_counter = (int*) malloc(sizeof(int) * ra_sen_cont);
			memset(wet_alert_counter,0,sizeof(int) * ra_sen_cont);
			wet_last_alert_counter = (int*) malloc(sizeof(int) * ra_sen_cont);
			memset(wet_last_alert_counter,0,sizeof(int) * ra_sen_cont);

			// Initialize adc1 width
			adc1_config_width(ra_sen_width);

			// Initialize alerts
			ra_sen_alerts.wet_alert = false;
			ra_sen_alerts.wet_ticks_to_alert = 10;
			ra_sen_alerts.wet_upper_threshold = 1000;
			ra_sen_alerts.wet_lower_threshold = -1000;

			// Get alert values
			nvs_app_get_blob_value(nvs_RA_SEN_ALERTS_key,NULL,&size);
			if(size != 0)
				nvs_app_get_blob_value(nvs_RA_SEN_ALERTS_key,&ra_sen_alerts,&size);

			// In case of "cont" > 0 load extra information
			if(ra_sen_cont > 0){
				char dump[100];
				size = 0;

				nvs_app_get_blob_value(nvs_RA_SEN_GPIOS_key,NULL,&size);
				if(size != 0)
					nvs_app_get_blob_value(nvs_RA_SEN_GPIOS_key,ra_sen_gpios_array,&size);

				int gpios[RA_SEN_N_GPIOS];
				for(int i = 0; i < ra_sen_cont; i++){
					gpios[0] = ra_sen_gpios_array[i].a0;
					gpios_manager_lock_gpios(gpios, RA_SEN_N_GPIOS, dump);
					adc1_config_channel_atten(gpio_into_channel(gpios[0]), ra_sen_atten);
				}

				char key[15];
				char num[3];

				for(int i = 0; i < ra_sen_cont; i++){
					size = 0;
					strcpy(key, nvs_RA_SEN_LOCATIONS_key);
					sprintf(num, "%d", i);
					strcat(key,num);

					nvs_app_get_string_value(key,NULL,&size);
					if(size != 0)
						nvs_app_get_string_value(key,ra_sen_locations_array[i],&size);
				}
			}

			xTaskCreatePinnedToCore(&ra_sen_task, "ra_sen_task", RA_SEN_STACK_SIZE, NULL, RA_SEN_PRIORITY, &task_ra_sen, RA_SEN_CORE_ID);

			g_ra_sen_initialized = true;

			ESP_LOGI(TAG, "RA-SEN successfully registered");
		}
		else{
			ESP_LOGE(TAG, "Error, RA-SEN recollecter or sensor manager hasn't been registered");
		}
	}
}

int ra_sen_add_sensor(int* gpios, union sensor_value_u* parameters, char* reason){
	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");
		sprintf(reason, "RA-SEN not initialized");

		return -1;
	}

	adc1_channel_t channel = gpio_into_channel(gpios[0]);

	if(channel == -1){
		ESP_LOGE(TAG, "GPIO %d is not and adc1 type", gpios[0]);
		sprintf(reason, "GPIO %d is not and adc1 type", gpios[0]);

		return -1;
	}

	if(gpios_manager_lock_gpios(gpios, RA_SEN_N_GPIOS, reason) == -1)
		return -1;

	pthread_mutex_lock(&mutex_ra_sen);

	adc1_config_channel_atten(channel, ra_sen_atten);

	ra_sen_cont++;

	ra_sen_gpios_array = (ra_sen_gpios_t*) realloc(ra_sen_gpios_array, sizeof(ra_sen_gpios_t) * ra_sen_cont);
	ra_sen_data_array = (ra_sen_data_t*) realloc(ra_sen_data_array, sizeof(ra_sen_data_t) * ra_sen_cont);
	ra_sen_locations_array = (char**) realloc(ra_sen_locations_array, sizeof(char*) * ra_sen_cont);

	ra_sen_locations_array[ra_sen_cont - 1] = (char*) malloc(CHAR_LENGTH + 1);
	memset(ra_sen_locations_array[ra_sen_cont - 1],0,CHAR_LENGTH + 1);

	wet_alert_counter = (int*) realloc(wet_alert_counter, sizeof(int) * ra_sen_cont);
	wet_last_alert_counter = (int*) realloc(wet_last_alert_counter, sizeof(int) * ra_sen_cont);

	ra_sen_gpios_array[ra_sen_cont - 1].a0 = gpios[0];

	ra_sen_data_array[ra_sen_cont - 1].wet_percentage = 0.0;

	strcpy(ra_sen_locations_array[ra_sen_cont - 1], "");

	wet_alert_counter[ra_sen_cont - 1] = 0;
	wet_last_alert_counter[ra_sen_cont - 1] = 0;

	nvs_app_set_uint8_value(nvs_RA_SEN_CONT_key,(uint8_t)ra_sen_cont);
	nvs_app_set_blob_value(nvs_RA_SEN_GPIOS_key,ra_sen_gpios_array,sizeof(ra_sen_gpios_t)*ra_sen_cont);

	char key[15];
	char num[3];

	for(int i = 0; i < ra_sen_cont; i++){
		strcpy(key, nvs_RA_SEN_LOCATIONS_key);
		sprintf(num, "%d", i);
		strcat(key,num);

		nvs_app_set_string_value(key,ra_sen_locations_array[i]);
	}

	pthread_mutex_unlock(&mutex_ra_sen);

	ESP_LOGI(TAG, "Sensor installed");

	return 1;
}

int ra_sen_delete_sensor(int pos, char* reason){
	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");
		sprintf(reason, "RA-SEN not initialized");

		return -1;
	}

	if(!check_valid_pos(pos)){
		ESP_LOGE(TAG, "Position not valid");
		sprintf(reason, "Position not valid");

		return -1;
	}

	if(!check_free_gpios(pos, reason))
		return -1;

	pthread_mutex_lock(&mutex_ra_sen);

	for(int i = pos; i < ra_sen_cont - 1; i++){

		ra_sen_gpios_array[pos].a0 = ra_sen_gpios_array[pos + 1].a0;

		ra_sen_data_array[pos].wet_percentage = ra_sen_data_array[pos + 1].wet_percentage;

		strcpy(ra_sen_locations_array[pos], ra_sen_locations_array[pos + 1]);

		wet_alert_counter[pos] = wet_alert_counter[pos + 1];
		wet_last_alert_counter[pos] = wet_last_alert_counter[pos + 1];
	}

	ra_sen_cont--;

	ra_sen_gpios_array = (ra_sen_gpios_t*) realloc(ra_sen_gpios_array, sizeof(ra_sen_gpios_t) * ra_sen_cont);
	ra_sen_data_array = (ra_sen_data_t*) realloc(ra_sen_data_array, sizeof(ra_sen_data_t) * ra_sen_cont);

	free(ra_sen_locations_array[ra_sen_cont]);
	ra_sen_locations_array = (char**) realloc(ra_sen_locations_array, sizeof(char*) * ra_sen_cont);

	wet_alert_counter = (int*) realloc(wet_alert_counter, sizeof(int) * ra_sen_cont);
	wet_last_alert_counter = (int*) realloc(wet_last_alert_counter, sizeof(int) * ra_sen_cont);

	nvs_app_set_uint8_value(nvs_RA_SEN_CONT_key,(uint8_t)ra_sen_cont);
	nvs_app_set_blob_value(nvs_RA_SEN_GPIOS_key,ra_sen_gpios_array,sizeof(ra_sen_gpios_t)*ra_sen_cont);

	char key[15];
	char num[3];

	for(int i = 0; i < ra_sen_cont; i++){
		strcpy(key, nvs_RA_SEN_LOCATIONS_key);
		sprintf(num, "%d", i);
		strcat(key,num);

		nvs_app_set_string_value(key,ra_sen_locations_array[i]);
	}

	pthread_mutex_unlock(&mutex_ra_sen);

	ESP_LOGI(TAG, "Sensor deleted");

	return 1;
}

int ra_sen_set_gpios(int pos, int* gpios, char* reason){
	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");
		sprintf(reason, "RA-SEN not initialized");

		return -1;
	}

	if(!check_valid_pos(pos)){
		ESP_LOGE(TAG, "Position not valid");
		sprintf(reason, "Position not valid");

		return -1;
	}

	adc1_channel_t channel = gpio_into_channel(gpios[0]);

	if(channel == -1){
		ESP_LOGE(TAG, "GPIO %d is not and adc1 type", gpios[0]);
		sprintf(reason, "GPIO %d is not and adc1 type", gpios[0]);

		return -1;
	}

	if(gpios_manager_lock_gpios(gpios, RA_SEN_N_GPIOS, reason) == -1)
		return -1;

	if(!check_free_gpios(pos, reason))
		return -1;

	pthread_mutex_lock(&mutex_ra_sen);

	adc1_config_channel_atten(channel, ra_sen_atten);

	ra_sen_gpios_array[pos].a0 = gpios[0];

	nvs_app_set_blob_value(nvs_RA_SEN_GPIOS_key,ra_sen_gpios_array,sizeof(ra_sen_gpios_t)*ra_sen_cont);

	pthread_mutex_unlock(&mutex_ra_sen);

	return 1;
}

int ra_sen_set_parameters(int pos, union sensor_value_u* parameters, char* reason){
	sprintf(reason, "RA-SEN doesn't have parameters");
	return -1;
}

int ra_sen_set_location(int pos, char* location, char* reason){
	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");
		sprintf(reason, "RA-SEN not initialized");

		return -1;
	}

	if(!check_valid_pos(pos)){
		ESP_LOGE(TAG, "Position not valid");
		sprintf(reason, "Position not valid");

		return -1;
	}

	if(strlen(location) >= CHAR_LENGTH){
		ESP_LOGE(TAG, "Location too long (20 characters max)");
		sprintf(reason, "Location too long (20 characters max)");

		return -1;
	}

	pthread_mutex_lock(&mutex_ra_sen);

	strcpy(ra_sen_locations_array[pos], location);

	char key[15];
	char num[3];

	for(int i = 0; i < ra_sen_cont; i++){
		strcpy(key, nvs_RA_SEN_LOCATIONS_key);
		sprintf(num, "%d", i);
		strcat(key,num);

		nvs_app_set_string_value(key,ra_sen_locations_array[i]);
	}

	pthread_mutex_unlock(&mutex_ra_sen);

	return 1;
}

int ra_sen_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold, char* reason){
	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");
		sprintf(reason, "RA-SEN not initialized");

		return -1;
	}

	if(value < 0 || value > 0){
		ESP_LOGE(TAG, "Value doesn't exist");
		sprintf(reason, "Value doesn't exist");

		return -1;
	}

	if(n_ticks <= 0){
		ESP_LOGE(TAG, "Ticks must be greater than 0");
		sprintf(reason, "Ticks must be greater than 0");

		return -1;
	}

	if(upper_threshold.fval <= lower_threshold.fval){
		ESP_LOGE(TAG, "Upper threshold should be greater than lower");
		sprintf(reason, "Upper threshold should be greater than lower");

		return -1;
	}

	pthread_mutex_lock(&mutex_ra_sen);

	ra_sen_alerts.wet_alert = alert;
	ra_sen_alerts.wet_ticks_to_alert = n_ticks;
	ra_sen_alerts.wet_upper_threshold = upper_threshold.fval;
	ra_sen_alerts.wet_lower_threshold = lower_threshold.fval;

	nvs_app_set_blob_value(nvs_RA_SEN_ALERTS_key,&ra_sen_alerts,sizeof(ra_sen_alerts_t));

	pthread_mutex_unlock(&mutex_ra_sen);

	return 1;
}

sensor_data_t* ra_sen_get_sensors_data(int* number_of_sensors){

	*number_of_sensors = 0;

	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");

		return NULL;
	}

	pthread_mutex_lock(&mutex_ra_sen);

	sensor_data_t* aux;
	sensor_value_t* aux2;

	if(ra_sen_cont == 0){
		aux = (sensor_data_t*) malloc(sizeof(sensor_data_t));
		aux2 = (sensor_value_t*) malloc(sizeof(sensor_value_t) * RA_SEN_N_VALUES);

		strcpy(aux[0].sensorName, "RA-SEN");
		strcpy(aux[0].sensorLocation, "N/A");
		aux[0].valuesLen = RA_SEN_N_VALUES;
		aux[0].sensor_values = aux2;

		aux[0].sensor_values[0].showOnLCD = RA_SEN_SHOW_WET_PERCENTAGE_ON_LCD;
		strcpy(aux[0].sensor_values[0].valueName,"Wet");
		aux[0].sensor_values[0].sensor_value_type = FLOAT;
		aux[0].sensor_values[0].alert = ra_sen_alerts.wet_alert;
		aux[0].sensor_values[0].ticks_to_alert = ra_sen_alerts.wet_ticks_to_alert;
		aux[0].sensor_values[0].upper_threshold.fval = ra_sen_alerts.wet_upper_threshold;
		aux[0].sensor_values[0].lower_threshold.fval = ra_sen_alerts.wet_lower_threshold;
	}
	else{
		*number_of_sensors = ra_sen_cont;

		aux = (sensor_data_t*) malloc(sizeof(sensor_data_t) * ra_sen_cont);

		for(int i = 0; i < ra_sen_cont; i++){

			aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * RA_SEN_N_VALUES);

			strcpy(aux[i].sensorName, "RA-SEN");
			strcpy(aux[i].sensorLocation, ra_sen_locations_array[i]);
			aux[i].valuesLen = RA_SEN_N_VALUES;
			aux[i].sensor_values = aux2;

			aux[i].sensor_values[0].showOnLCD = RA_SEN_SHOW_WET_PERCENTAGE_ON_LCD;
			strcpy(aux[i].sensor_values[0].valueName,"Wet");
			aux[i].sensor_values[0].sensor_value_type = FLOAT;
			aux[i].sensor_values[0].sensor_value.fval = ra_sen_data_array[i].wet_percentage;
			aux[i].sensor_values[0].alert = ra_sen_alerts.wet_alert;
			aux[i].sensor_values[0].ticks_to_alert = ra_sen_alerts.wet_ticks_to_alert;
			aux[i].sensor_values[0].upper_threshold.fval = ra_sen_alerts.wet_upper_threshold;
			aux[i].sensor_values[0].lower_threshold.fval = ra_sen_alerts.wet_lower_threshold;
		}
	}

	pthread_mutex_unlock(&mutex_ra_sen);

	return aux;
}

sensor_gpios_info_t* ra_sen_get_sensors_gpios(int* number_of_sensors){

	*number_of_sensors = 0;

	if(!g_ra_sen_initialized){
		ESP_LOGE(TAG, "RA-SEN not initialized");

		return NULL;
	}

	pthread_mutex_lock(&mutex_ra_sen);

	sensor_gpios_info_t* aux;
	sensor_gpio_t* aux2;

	if(ra_sen_cont == 0){
		aux = (sensor_gpios_info_t*) malloc(sizeof(sensor_gpios_info_t));
		aux2 = (sensor_gpio_t *)malloc(sizeof(sensor_gpio_t) * RA_SEN_N_GPIOS);

		aux[0].gpiosLen = RA_SEN_N_GPIOS;
		aux[0].sensor_gpios = aux2;

		strcpy(aux[0].sensor_gpios[0].gpioName,"A0");
	}
	else{
		*number_of_sensors = ra_sen_cont;

		aux = (sensor_gpios_info_t*) malloc(sizeof(sensor_gpios_info_t) * ra_sen_cont);

		for(int i = 0; i < ra_sen_cont; i++){

			aux2 = (sensor_gpio_t *)malloc(sizeof(sensor_gpio_t) * RA_SEN_N_GPIOS);

			aux[i].gpiosLen = RA_SEN_N_GPIOS;
			aux[i].sensor_gpios = aux2;

			strcpy(aux[i].sensor_gpios[0].gpioName,"A0");
			aux[i].sensor_gpios[0].sensor_gpio = ra_sen_gpios_array[i].a0;
		}
	}

	pthread_mutex_unlock(&mutex_ra_sen);

	return aux;
}

sensor_additional_parameters_info_t* ra_sen_get_sensors_additional_parameters(int* number_of_sensors){
	*number_of_sensors = 0;
	return NULL;
}
