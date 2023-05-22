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
#include "gpios/ra_sen.h"

static const char TAG[] 							= "sensors_manager";

bool g_sensors_manager_initialized 					= false;

static add_unit_function *add_unit;
static delete_unit_function *delete_unit;

static set_gpios_function *set_gpios;
static set_parameters_function *set_parameters;
static set_location_function *set_location;
static set_alert_values_function *set_alert_values;

static int sensors_manager_cont;

static pthread_mutex_t mutex_sensors_manager;

/**
 * Check if id is valid
 */
static bool check_valid_id(int id){
	bool valid;

	pthread_mutex_lock(&mutex_sensors_manager);

	if(id < 0 || id >= sensors_manager_cont)
		valid = false;
	else
		valid = true;

	pthread_mutex_unlock(&mutex_sensors_manager);

	return valid;
}

void sensors_manager_init(void){
	if(g_sensors_manager_initialized == true)
		ESP_LOGE(TAG, "Sensors manager already initialized");
	else if(pthread_mutex_init(&mutex_sensors_manager, NULL) != 0)
		ESP_LOGE(TAG,"Failed to initialize the sensors manager mutex");
	else{
		add_unit = (add_unit_function*) malloc(sizeof(add_unit_function) * 1);
		delete_unit = (delete_unit_function*) malloc(sizeof(delete_unit_function) * 1);

		set_gpios = (set_gpios_function*) malloc(sizeof(set_gpios_function) * 1);
		set_parameters = (set_parameters_function*) malloc(sizeof(set_parameters_function) * 1);
		set_location = (set_location_function*) malloc(sizeof(set_location_function) * 1);
		set_alert_values = (set_alert_values_function*) malloc(sizeof(set_alert_values_function) * 1);

		sensors_manager_cont = 0;

		g_sensors_manager_initialized = true;
	}
}

int sensors_manager_add(add_unit_function aun, delete_unit_function dun, set_gpios_function sgp, set_parameters_function spa, set_location_function slo, set_alert_values_function sal){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	sensors_manager_cont++;

	add_unit = (add_unit_function*) realloc(add_unit, sizeof(add_unit_function) * sensors_manager_cont);
	delete_unit = (delete_unit_function*) realloc(delete_unit, sizeof(delete_unit_function) * sensors_manager_cont);

	set_gpios = (set_gpios_function*) realloc(set_gpios, sizeof(set_gpios_function) * sensors_manager_cont);
	set_parameters = (set_parameters_function*) realloc(set_parameters, sizeof(set_parameters_function) * sensors_manager_cont);
	set_location = (set_location_function*) realloc(set_location, sizeof(set_location_function) * sensors_manager_cont);
	set_alert_values = (set_alert_values_function*) realloc(set_alert_values, sizeof(set_alert_values_function) * sensors_manager_cont);

	add_unit[sensors_manager_cont - 1] = aun;
	delete_unit[sensors_manager_cont - 1] = dun;

	set_gpios[sensors_manager_cont - 1] = sgp;
	set_parameters[sensors_manager_cont - 1] = spa;
	set_location[sensors_manager_cont - 1] = slo;
	set_alert_values[sensors_manager_cont - 1] = sal;

	pthread_mutex_unlock(&mutex_sensors_manager);

	ESP_LOGI(TAG, "Sensor manager added");

	return 1;
}


void sensors_manager_sensors_initialization(void){
	mq2_init();
	dht22_init();
	hc_rs04_init();
	so_sen_init();
	ra_sen_init();
}

int sensors_manager_add_sensor_unit(int id, int* gpios, union sensor_value_u* parameters, char* reason){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");
		sprintf(reason, "Sensors manager not initialized");

		return -1;
	}

	if(!check_valid_id(id)){
		ESP_LOGE(TAG, "Sensor id not valid");
		sprintf(reason, "Sensor id not valid");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	add_unit_function foo = add_unit[id];

	int res = foo(gpios, parameters, reason);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_delete_sensor_unit(int id, int pos, char* reason){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");
		sprintf(reason, "Sensors manager not initialized");

		return -1;
	}

	if(!check_valid_id(id)){
		ESP_LOGE(TAG, "Sensor id not valid");
		sprintf(reason, "Sensor id not valid");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	delete_unit_function foo = delete_unit[id];

	int res = foo(pos, reason);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_set_gpios(int id, int pos, int* gpios, char* reason){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");
		sprintf(reason, "Sensors manager not initialized");

		return -1;
	}

	if(!check_valid_id(id)){
		ESP_LOGE(TAG, "Sensor id not valid");
		sprintf(reason, "Sensor id not valid");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	set_gpios_function foo = set_gpios[id];

	int res = foo(pos, gpios, reason);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_set_parameters(int id, int pos, union sensor_value_u* parameters, char* reason){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");
		sprintf(reason, "Sensors manager not initialized");

		return -1;
	}

	if(!check_valid_id(id)){
		ESP_LOGE(TAG, "Sensor id not valid");
		sprintf(reason, "Sensor id not valid");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	set_parameters_function foo = set_parameters[id];

	int res = foo(pos, parameters, reason);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_set_location(int id, int pos, char* location, char* reason){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");
		sprintf(reason, "Sensors manager not initialized");

		return -1;
	}

	if(!check_valid_id(id)){
		ESP_LOGE(TAG, "Sensor id not valid");
		sprintf(reason, "Sensor id not valid");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	set_location_function foo = set_location[id];

	int res = foo(pos, location, reason);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}

int sensors_manager_set_alert_values(int id, int value, bool alert, int ticks, union sensor_value_u upperThreshold, union sensor_value_u lowerThreshold, char* reason){
	if(!g_sensors_manager_initialized){
		ESP_LOGE(TAG, "Sensors manager not initialized");
		sprintf(reason, "Sensors manager not initialized");

		return -1;
	}

	if(!check_valid_id(id)){
		ESP_LOGE(TAG, "Sensor id not valid");
		sprintf(reason, "Sensor id not valid");

		return -1;
	}

	pthread_mutex_lock(&mutex_sensors_manager);

	set_alert_values_function foo = set_alert_values[id];

	int res = foo(value, alert, ticks, upperThreshold, lowerThreshold, reason);

	pthread_mutex_unlock(&mutex_sensors_manager);

	return res;
}
