/*
 * gpios.c
 *
 *  Created on: 28 feb. 2023
 *      Author: Kike
 */

#include "gpios_manager.h"

#include <pthread.h>

#include "unistd.h"
#include "esp_log.h"
#include "string.h"
#include "recollecter.h"

static const char TAG[]  = "gpios";

static const int original_gpios[] = {36,39,34,35,32,33,25,26,27,14,12,13,9,10,11,23,22,1,3,21,19,18,5,4,0,2,15,8,7,6};
#define ORIGINAL_CONT 30

bool g_gpios_initialized = false;

static int gpios_list[ORIGINAL_CONT];
static int cont;

static pthread_mutex_t mutex_gpios;

/**
 * Check if one GPIO can be selected multiple times
 */
static bool check_multiple_times(int gpio){

	if(gpio == 21 || gpio == 22) // I2C GPIOS
		return true;
	else
		return false;
}

/**
 * Check if GPIOS are repeated
 */
static bool check_repeated(int* gpios, int size){
	bool repeated = false;
	int i = 0;
	int j;

	while(i < (size - 1) && !repeated){
		j = i + 1;
		while(j < size && !repeated){
			if(gpios[i] == gpios[j]){
				repeated = true;
			}
			j++;
		}
		i++;
	}

	return repeated;
}

/**
 * Check if GPIOS are original
 */
static bool check_originals(int* gpios, int size){
	bool originals = true, found;
	int i = 0, j;

	while(i < size && originals){
		found = false;
		j = 0;

		while(j < ORIGINAL_CONT && !found){
			if(gpios[i] == original_gpios[j])
				found = true;
			else
				j++;
		}

		if(!found)
			originals = false;
		else
			i++;
	}

	return originals;
}

/**
 * Check if all GPIOS are free
 */
static bool check_all_free(int* gpios, int size){
	bool free = true, found;
	int i = 0, j;

	pthread_mutex_lock(&mutex_gpios);

	while(i < size && free){
		found = false;
		j = 0;

		while(j < cont && !found){
			if(gpios[i] == gpios_list[j])
				found = true;
			else
				j++;
		}

		if(!found)
			free = false;
		else
			i++;
	}

	pthread_mutex_unlock(&mutex_gpios);

	return free;
}

/**
 * Check if one of the GPIOS is free
 */
static bool check_one_free(int* gpios, int size){
	bool free = false, found;
	int i = 0, j;

	pthread_mutex_lock(&mutex_gpios);

	while(i < size && !free){
		found = false;
		j = 0;

		if(!check_multiple_times(gpios[i])){
			while(j < cont && !found){
				if(gpios[i] == gpios_list[j])
					found = true;
				else
					j++;
			}
		}

		if(found)
			free = true;
		else
			i++;
	}

	pthread_mutex_unlock(&mutex_gpios);

	return free;
}

/**
 * Get index of a GPIO
 */
static int get_index(int gpio){
	int index = 0;
	bool found = false;

	while(index < cont && !found){
		if(gpios_list[index] == gpio)
			found = true;
		else
			index++;
	}

	return index;
}

void gpios_manager_init(void){

	if(g_gpios_initialized == true)
		ESP_LOGE(TAG, "GPIOS manager already initialized");
	else if(pthread_mutex_init(&mutex_gpios, NULL) != 0)
		ESP_LOGE(TAG, "Failed to initialize the GPIOS mutex");
	else{

		for(int i = 0; i < ORIGINAL_CONT; i++){
			gpios_list[i] = original_gpios[i];
		}
		cont = ORIGINAL_CONT;

		g_gpios_initialized = true;
	}
}

int gpios_manager_lock_gpios(int* gpios, int size, char* reason){

	if(g_gpios_initialized == false){
		ESP_LOGE(TAG, "GPIOS manager not initialized");
		sprintf(reason, "GPIOS manager not initialized");

		return -1;
	}

	if(check_repeated(gpios, size)){
		ESP_LOGE(TAG, "Same GPIOS multiple times");
		sprintf(reason, "Same GPIOS multiple times");

		return -1;
	}

	if(!check_all_free(gpios, size)){
		ESP_LOGE(TAG, "Some GPIO is not free");
		sprintf(reason, "Some GPIO is not free");

		return -1;
	}

	pthread_mutex_lock(&mutex_gpios);

	int index;

	for(int i = 0; i < size; i++){

		if(!check_multiple_times(gpios[i])){
			index = get_index(gpios[i]);

			for(int j = index; j < cont - 1; j++){
				gpios_list[j] = gpios_list[j + 1];
			}

			cont--;
		}
	}

	pthread_mutex_unlock(&mutex_gpios);

	return 1;
}


int gpios_manager_free_gpios(int* gpios, int size, char* reason){

	if(g_gpios_initialized == false){
		ESP_LOGE(TAG, "GPIOS manager not initialized");
		sprintf(reason, "GPIOS manager not initialized");

		return -1;
	}

	if(check_repeated(gpios, size)){
		ESP_LOGE(TAG, "Same GPIOS multiple times");
		sprintf(reason, "Same GPIOS multiple times");

		return -1;
	}

	if(!check_originals(gpios, size)){
		ESP_LOGE(TAG, "Some GPIO is not original");
		sprintf(reason, "Some GPIO is not original");

		return -1;
	}

	if(check_one_free(gpios, size)){
		ESP_LOGE(TAG, "Some GPIO is already free");
		sprintf(reason, "Some GPIO is already free");

		return -1;
	}

	pthread_mutex_lock(&mutex_gpios);

	for(int i = 0; i < size; i++){
		if(!check_multiple_times(gpios[i])){
			gpios_list[cont] = gpios[i];
			cont++;
		}
	}

	pthread_mutex_unlock(&mutex_gpios);

	return 1;
}

int* gpios_manager_get_gpios_availables(int* number_of_gpios){

	*number_of_gpios = 0;

	if(g_gpios_initialized == false){
		ESP_LOGE(TAG, "GPIOS manager not initialized");

		return NULL;
	}

	pthread_mutex_lock(&mutex_gpios);

	int* gpios;
	gpios = (int*) malloc(sizeof(int) * cont);

	*number_of_gpios = cont;

	for(int i = 0; i < cont; i++){
		gpios[i] = gpios_list[i];
	}

	pthread_mutex_unlock(&mutex_gpios);

	return gpios;
}
