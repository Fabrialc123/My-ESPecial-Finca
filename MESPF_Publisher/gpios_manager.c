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

bool g_gpios_initialized = false;

static int gpios_list[ORIGINAL_CONT];
static int cont;

static pthread_mutex_t mutex_gpios;

/**
 * Check if a gpio is repeated (brute force)
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
 * Delete the gpios sent as parameter
 */
static void delete_shifting(int* gpios, int size){
	int index;
	bool found;

	pthread_mutex_lock(&mutex_gpios);

	for(int i = 0; i < size; i++){

		index = 0;
		found = false;

		// Search the index of the GPIO
		while(index < cont && !found){
			if(gpios_list[index] == gpios[i]){
				found = true;
			}
			else{
				index++;
			}
		}

		// Shift and delete the GPIO
		for(int i = index; i < cont - 1; i++){
			gpios_list[i] = gpios_list[i + 1];
		}

		// Shorten the list
		cont--;
	}

	pthread_mutex_unlock(&mutex_gpios);
}

void gpios_manager_init(void){

	if(pthread_mutex_init(&mutex_gpios, NULL) != 0){
		ESP_LOGE(TAG, "Failed to initialize the GPIOS mutex");
	}
	else{

		for(int i = 0; i < ORIGINAL_CONT; i++){
			gpios_list[i] = original_gpios[i];
		}
		cont = ORIGINAL_CONT;

		g_gpios_initialized = true;
	}
}

int gpios_manager_lock(int* gpios, int size){

	int i = 0;
	int res = 1;

	// Look if gpios are not repeated
	if(check_repeated(gpios, size)){
		ESP_LOGE(TAG, "you can't pick the same gpios multiple times");
		res = -1;
	}

	// Look if all GPIOS passed are available
	while(i < size && res == 1){
		if(!gpios_manager_is_free(gpios[i])){
			ESP_LOGE(TAG, "gpios passed are not available!");
			res = -1;
		}
		i++;
	}

	// In case they are, delete them from the array
	if(res == 1){
		delete_shifting(gpios, size);
	}

	return res;
}


int gpios_manager_free(int* gpios, int size){

	int i;
	int res = 1;
	bool found[size];

	// Look if gpios are not repeated
	if(check_repeated(gpios, size)){
		ESP_LOGE(TAG, "you can't pick the same gpios multiple times");
		res = -1;
	}
	else{

		// Initialize found array
		for(i = 0; i < size; i++){
			found[i] = false;
		}

		// Look if the gpios passed were originally a possibility
		for(int g = 0; g < size; g++){
			i = 0;
			while(i < ORIGINAL_CONT && !found[g]){
				if(gpios[g] == original_gpios[i]){
					found[g] = true;
				}
				else{
					i++;
				}
			}
		}

		// Check if all are true
		i = 0;

		while(i < size && found[i] == true){
			i++;
		}

		if(i < size){
			ESP_LOGE(TAG, "gpios passed couldn't be an option!");
			res = -1;
		}
		else{
			i = 0;

			// Look if all GPIOS passed are not available
			while(i < size && res == 1){
				if(gpios_manager_is_free(gpios[i])){
					ESP_LOGE(TAG, "gpios passed are available!");
					res = -1;
				}
				i++;
			}

			pthread_mutex_lock(&mutex_gpios);

			// In case they are not, add them to the array
			if(res == 1){
				for(i = 0; i < size; i++){
					gpios_list[cont] = gpios[i];
					cont++;
				}
			}

			pthread_mutex_unlock(&mutex_gpios);
		}
	}

	return res;
}


bool gpios_manager_is_free(int gpio){

	int i = 0;
	bool found = false;

	pthread_mutex_lock(&mutex_gpios);

	while(i < cont && !found){

		if(gpio == gpios_list[i]){
			found = true;
		}
		else{
			i++;
		}
	}

	pthread_mutex_unlock(&mutex_gpios);

	return found;
}

int gpios_manager_get_cont(void){
	int size = 0;

	pthread_mutex_lock(&mutex_gpios);

	size = cont;

	pthread_mutex_unlock(&mutex_gpios);

	return size;
}

int* gpios_manager_get_availables(void){
	int* gpios;

	gpios = (int*) malloc(sizeof(int) * cont);

	pthread_mutex_lock(&mutex_gpios);

	for(int i = 0; i < cont; i++){
		gpios[i] = gpios_list[i];
	}

	pthread_mutex_unlock(&mutex_gpios);

	return gpios;
}

void gpios_manager_json(char *data){

	char aux[CHAR_LENGTH];

	int numOfGpiosAvailables = cont;
	int* gpiosAvailables = gpios_manager_get_availables();

	strcat(data, "{\"numberOfGpiosAvailables\":");
	memset(&aux, 0, CHAR_LENGTH);
	sprintf(aux, "%d", numOfGpiosAvailables);
	strcat(data, aux);

	strcat(data, ", \"gpios\":[");

	for(int i = 0; i < numOfGpiosAvailables; i++){

		if(i > 0)
			strcat(data, ",");

		memset(&aux, 0, CHAR_LENGTH);
		sprintf(aux, "%d", gpiosAvailables[i]);
		strcat(data, aux);
	}

	strcat(data, "]}");
}

bool gpios_manager_check_adc1(int gpio){
	switch (gpio) {
		case 36: case 37: case 38: case 39: case 32: case 33: case 34: case 35:
			return true;
		default:
			return false;
	}
}
