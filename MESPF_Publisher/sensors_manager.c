/*
 * sensors_manager.c
 *
 *  Created on: 12 mar. 2023
 *      Author: Kike
 */

#include "esp_log.h"
#include "string.h"

#include "sensors_manager.h"
#include "gpios_manager.h"
#include <stdbool.h>
#include <pthread.h>

#include "gpios/mq2.h"
#include "gpios/dht22.h"
#include "gpios/hc_rs04.h"
#include "gpios/so_sen.h"

static const char TAG[] 							= "sensors_manager";

bool g_sensors_manager_initialized 					= false;

static destroy_sensor_function *destroy_sensor;
static add_unit_function *add_unit;
static delete_unit_function *delete_unit;
static set_gpios_function *set_gpios;
static set_parameters_function *set_parameters;
static set_alert_values_function *set_alert_values;
static int sensors_manager_n;

static pthread_mutex_t mutex_sensors_manager;

void sensors_manager_sensors_startup(void){
	mq2_startup();
	dht22_startup();
	hc_rs04_startup();
	so_sen_startup();
}

void sensors_manager_validate_info(int type, int *gpios, union sensor_value_u *parameters, char *response){

	switch (type) {
		case 0:{
			if(!gpios_manager_is_free(gpios[0]))
				sprintf(response,"GPIO %d not available", gpios[0]);
			else if(!gpios_manager_check_adc1(gpios[0]))
				sprintf(response,"GPIO %d is not an ADC1", gpios[0]);
			else
				sprintf(response,"Info is valid");
		}break;
		case 1:{
			if(!gpios_manager_is_free(gpios[0]))
				sprintf(response,"GPIO %d not available", gpios[0]);
			else
				sprintf(response,"Info is valid");
		}break;
		case 2:{
			if(!gpios_manager_is_free(gpios[0]))
				sprintf(response,"GPIO %d not available", gpios[0]);
			else if(!gpios_manager_is_free(gpios[1]))
				sprintf(response,"GPIO %d not available", gpios[1]);
			else if(parameters[0].ival <= 0 || parameters[1].ival <= 0)
				sprintf(response,"Params must be more than 0");
			else
				sprintf(response,"Info is valid");
		}break;
		case 3:{
			if(!gpios_manager_is_free(gpios[0]))
				sprintf(response,"GPIO %d not available", gpios[0]);
			else if(!gpios_manager_check_adc1(gpios[0]))
				sprintf(response,"GPIO %d is not an ADC1", gpios[0]);
			else
				sprintf(response,"Info is valid");
		}break;
		default:{
			sprintf(response, "Unknown type");
		}break;
	}
}

void sensors_manager_init(void){
	if(pthread_mutex_init (&mutex_sensors_manager, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the sensors manager mutex");
	}

	destroy_sensor = (destroy_sensor_function*) malloc(sizeof(destroy_sensor_function) * 1);
	add_unit = (add_unit_function*) malloc(sizeof(add_unit_function) * 1);
	delete_unit = (delete_unit_function*) malloc(sizeof(delete_unit_function) * 1);
	set_gpios = (set_gpios_function*) malloc(sizeof(set_gpios_function) * 1);
	set_parameters = (set_parameters_function*) malloc(sizeof(set_parameters_function) * 1);
	set_alert_values = (set_alert_values_function*) malloc(sizeof(set_alert_values_function) * 1);
	sensors_manager_n = 0;

	g_sensors_manager_initialized = true;
}

int sensors_manager_add(destroy_sensor_function des, add_unit_function aun, delete_unit_function dun, set_gpios_function sgp, set_parameters_function spa, set_alert_values_function sal){

	int res = 1;

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	ESP_LOGI(TAG, "Sensor added");

	sensors_manager_n++;

	destroy_sensor = (destroy_sensor_function*) realloc(destroy_sensor, sizeof(destroy_sensor_function) * sensors_manager_n);
	add_unit = (add_unit_function*) realloc(add_unit, sizeof(add_unit_function) * sensors_manager_n);
	delete_unit = (delete_unit_function*) realloc(delete_unit, sizeof(delete_unit_function) * sensors_manager_n);
	set_gpios = (set_gpios_function*) realloc(set_gpios, sizeof(set_gpios_function) * sensors_manager_n);
	set_parameters = (set_parameters_function*) realloc(set_parameters, sizeof(set_parameters_function) * sensors_manager_n);
	set_alert_values = (set_alert_values_function*) realloc(set_alert_values, sizeof(set_alert_values_function) * sensors_manager_n);

	destroy_sensor[sensors_manager_n - 1] = des;
	add_unit[sensors_manager_n - 1] = aun;
	delete_unit[sensors_manager_n - 1] = dun;
	set_gpios[sensors_manager_n - 1] = sgp;
	set_parameters[sensors_manager_n - 1] = spa;
	set_alert_values[sensors_manager_n - 1] = sal;

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_delete(int id){

	int res = 1;

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	ESP_LOGI(TAG, "Sensor deleted");

	for(int i = id; i < sensors_manager_n - 1; i++){

		destroy_sensor[i] = destroy_sensor[i+1];
		add_unit[i] = add_unit[i+1];
		delete_unit[i] = delete_unit[i+1];
		set_gpios[i] = set_gpios[i+1];
		set_parameters[i] = set_parameters[i+1];
		set_alert_values[i] = set_alert_values[i+1];
	}

	sensors_manager_n--;

	destroy_sensor = (destroy_sensor_function*) realloc(destroy_sensor, sizeof(destroy_sensor_function) * sensors_manager_n);
	add_unit = (add_unit_function*) realloc(add_unit, sizeof(add_unit_function) * sensors_manager_n);
	delete_unit = (delete_unit_function*) realloc(delete_unit, sizeof(delete_unit_function) * sensors_manager_n);
	set_gpios = (set_gpios_function*) realloc(set_gpios, sizeof(set_gpios_function) * sensors_manager_n);
	set_parameters = (set_parameters_function*) realloc(set_parameters, sizeof(set_parameters_function) * sensors_manager_n);
	set_alert_values = (set_alert_values_function*) realloc(set_alert_values, sizeof(set_alert_values_function) * sensors_manager_n);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_size(void){
	int size = 0;

	pthread_mutex_lock(&mutex_sensors_manager);
	size = sensors_manager_n;
	pthread_mutex_unlock(&mutex_sensors_manager);

	return size;
}

int sensors_manager_init_sensor(int type){

	switch (type) {
		case 0:{
			mq2_init();
		}break;
		case 1:{
			dht22_init();
		}break;
		case 2:{
			hc_rs04_init();
		}break;
		case 3:{
			so_sen_init();
		}break;
		default:{
			return -1;
		}break;
	}

	return 1;
}

int sensors_manager_destroy_sensor(int id){

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	destroy_sensor_function foo = destroy_sensor[id];

	pthread_mutex_unlock(&mutex_sensors_manager);

	foo();

	sensors_manager_delete(id);
	delete_recollecter(id + 1);

	return 1;
}

int sensors_manager_add_sensor_unit(int id, int* gpios, union sensor_value_u* parameters){

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	add_unit_function foo = add_unit[id];

	pthread_mutex_unlock(&mutex_sensors_manager);

	return foo(gpios, parameters);
}

int sensors_manager_delete_sensor_unit(int id, int pos){

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	delete_unit_function foo = delete_unit[id];

	pthread_mutex_unlock(&mutex_sensors_manager);

	return foo(pos);
}

int sensors_manager_set_gpios(int id, int pos, int* gpios){

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	set_gpios_function foo = set_gpios[id];

	pthread_mutex_unlock(&mutex_sensors_manager);

	return foo(pos, gpios);
}

int sensors_manager_set_parameters(int id, int pos, union sensor_value_u* parameters){

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	set_parameters_function foo = set_parameters[id];

	pthread_mutex_unlock(&mutex_sensors_manager);

	return foo(pos, parameters);
}

int sensors_manager_set_alert_values(int id, int value, bool alert, int ticks, union sensor_value_u upperThreshold, union sensor_value_u lowerThreshold){

	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the sensors manager without initializing it");
		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_n){
		pthread_mutex_unlock(&mutex_sensors_manager);

		ESP_LOGE(TAG, "Error, inexistent id");
		return -1;
	}

	set_alert_values_function foo = set_alert_values[id];

	pthread_mutex_unlock(&mutex_sensors_manager);

	return foo(value, alert, ticks, upperThreshold, lowerThreshold);
}
