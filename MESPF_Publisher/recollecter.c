/*
 * recollecter.c
 *
 *  Created on: 27 sept. 2022
 *      Author: fabri
 */

#include "esp_log.h"
#include "string.h"
#include "cjson.h"

#include "recollecter.h"
#include <pthread.h>

static pthread_mutex_t mutex_RECOLLECTER;
static recollecter_function *recollecters;
static recollecter_gpios_function *recollecters_gpios;
static recollecter_parameters_function *recollecters_parameters;
static int recollecters_n;


static const char TAG[] = "RECOLLECTER";

int register_recollecter (recollecter_function rtr, recollecter_gpios_function rtg, recollecter_parameters_function rtp){
	int res = 0;

pthread_mutex_lock(&mutex_RECOLLECTER);
	 if (recollecters_n >= RECOLLECTER_SIZE){
		res = -1;
		ESP_LOGE(TAG, "ERROR in register_recollecter, the array is FULL (%d/%d)! ",recollecters_n, RECOLLECTER_SIZE);
	}
	else {
		recollecters[recollecters_n] = rtr;
		recollecters_gpios[recollecters_n] = rtg;
		recollecters_parameters[recollecters_n] = rtp;
		recollecters_n++;
		res = 1;
	}
pthread_mutex_unlock(&mutex_RECOLLECTER);

	return res;
}

int delete_recollecter (int id){
	int res = 0;

pthread_mutex_lock(&mutex_RECOLLECTER);
	 if (id >= recollecters_n || id < 0){
		res = -1;
		ESP_LOGE(TAG, "ERROR in delete_recollecter, id not valid (number of sensors in the system: %d)", recollecters_n);
	}
	else {

		for(int i = id; i < recollecters_n - 1; i++){
			recollecters[i] = recollecters[i + 1];
			recollecters_gpios[i] = recollecters_gpios[i + 1];
			recollecters_parameters[i] = recollecters_parameters[i + 1];
		}

		recollecters_n--;

		res = 1;
	}
pthread_mutex_unlock(&mutex_RECOLLECTER);

	return res;
}

int get_recollecters_size (void){
	int size = 0;

pthread_mutex_lock(&mutex_RECOLLECTER);
	size = recollecters_n;
pthread_mutex_unlock(&mutex_RECOLLECTER);

	return size;
}

void get_sensors_json(char *data){

	int number_of_sensors;
	sensor_data_t* sensor_data;
	sensor_gpios_info_t* sensor_gpios;
	sensor_additional_parameters_info_t* sensor_parameters;
	bool first = true;

	char aux[CHAR_LENGTH];

	strcpy(data, "{\"nSensors\":");

	memset(&aux, 0, CHAR_LENGTH);
	sprintf(aux, "%d", recollecters_n);
	strcat(data, aux);

	strcat(data, ",\"sensors\":[");

	for(int i = 0; i < recollecters_n; i++){

		sensor_data = get_sensor_data(i, &number_of_sensors);
		sensor_gpios = get_sensor_gpios(i, &number_of_sensors);
		sensor_parameters = get_sensor_parameters(i, &number_of_sensors);

		if(sensor_data != NULL){

			if(!first){
				strcat(data, ",");
			}

			strcat(data, "{\"sensorName\":\"");
			strcat(data, sensor_data[0].sensorName);
			strcat(data, "\"");

			strcat(data, ",\"numberOfUnits\":");
			memset(&aux, 0, CHAR_LENGTH);
			sprintf(aux, "%d", number_of_sensors);
			strcat(data, aux);

			strcat(data, ",\"numberOfGpios\":");

			if(sensor_gpios != NULL){
				memset(&aux, 0, CHAR_LENGTH);
				sprintf(aux, "%d", sensor_gpios[0].gpiosLen);
				strcat(data, aux);

				strcat(data, ",\"gpioNames\":[");
				for(int j = 0; j < sensor_gpios[0].gpiosLen; j++){
					strcat(data, "\"");
					strcat(data, sensor_gpios[0].sensor_gpios[j].gpioName);
					strcat(data, "\"");

					if(j < sensor_gpios[0].gpiosLen - 1){
						strcat(data, ",");
					}
				}
				strcat(data, "]");
			}
			else{
				memset(&aux, 0, CHAR_LENGTH);
				sprintf(aux, "%d", 0);
				strcat(data, aux);

				strcat(data, ",\"gpioNames\":null");
			}

			strcat(data, ",\"numberOfValues\":");
			memset(&aux, 0, CHAR_LENGTH);
			sprintf(aux, "%d", sensor_data[0].valuesLen);
			strcat(data, aux);

			strcat(data, ",\"valueNames\":[");
			for(int j = 0; j < sensor_data[0].valuesLen; j++){
				strcat(data, "\"");
				strcat(data, sensor_data[0].sensor_values[j].valueName);
				strcat(data, "\"");

				if(j < sensor_data[0].valuesLen - 1){
					strcat(data, ",");
				}
			}
			strcat(data, "]");

			strcat(data, ",\"valueTypes\":[");
			for(int j = 0; j < sensor_data[0].valuesLen; j++){
				strcat(data, "\"");

				if(sensor_data[0].sensor_values[j].sensor_value_type == INTEGER)
					strcat(data, "INTEGER");
				else if(sensor_data[0].sensor_values[j].sensor_value_type == FLOAT)
					strcat(data, "FLOAT");
				else
					strcat(data, "STRING");

				strcat(data, "\"");

				if(j < sensor_data[0].valuesLen - 1){
					strcat(data, ",");
				}
			}
			strcat(data, "]");

			strcat(data, ",\"numberOfParameters\":");

			if(sensor_parameters != NULL){
				memset(&aux, 0, CHAR_LENGTH);
				sprintf(aux, "%d", sensor_parameters[0].parametersLen);
				strcat(data, aux);

				strcat(data, ",\"parameterNames\":[");
				for(int j = 0; j < sensor_parameters[0].parametersLen; j++){
					strcat(data, "\"");
					strcat(data, sensor_parameters[0].sensor_parameters[j].parameterName);
					strcat(data, "\"");

					if(j < sensor_parameters[0].parametersLen - 1){
						strcat(data, ",");
					}
				}
				strcat(data, "]");

				strcat(data, ",\"parameterTypes\":[");
				for(int j = 0; j < sensor_parameters[0].parametersLen; j++){
					strcat(data, "\"");

					if(sensor_parameters[0].sensor_parameters[j].sensor_parameter_type == INTEGER)
						strcat(data, "INTEGER");
					else if(sensor_parameters[0].sensor_parameters[j].sensor_parameter_type == FLOAT)
						strcat(data, "FLOAT");
					else
						strcat(data, "STRING");

					strcat(data, "\"");

					if(j < sensor_parameters[0].parametersLen - 1){
						strcat(data, ",");
					}
				}
				strcat(data, "]");
			}
			else{
				memset(&aux, 0, CHAR_LENGTH);
				sprintf(aux, "%d", 0);
				strcat(data, aux);

				strcat(data, ",\"parameterNames\":null");
				strcat(data, ",\"parameterTypes\":null");
			}

			strcat(data, "}");

			if(first)
				first = false;
		}

		// Free memory
		if(sensor_data != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_data[j].sensor_values);
			}
			free(sensor_data);
		}
		if(sensor_gpios != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_gpios[j].sensor_gpios);
			}
			free(sensor_gpios);
		}
		if(sensor_parameters != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_parameters[j].sensor_parameters);
			}
			free(sensor_parameters);
		}
	}

	strcat(data, "]}");
}

void get_sensors_values_json(char *data){

	int number_of_sensors;
	sensor_data_t* sensor_data;
	bool first = true;

	char aux[CHAR_LENGTH];

	strcpy(data, "{");

	for(int i = 0; i < recollecters_n; i++){

		sensor_data = get_sensor_data(i, &number_of_sensors);

		if(sensor_data != NULL){

			if(!first){
				strcat(data, ",");
			}

			strcat(data, "\"");
			memset(&aux, 0, CHAR_LENGTH);
			sprintf(aux, "%d", i);
			strcat(data, aux);
			strcat(data, "\":[");

			for(int j = 0; j < number_of_sensors; j++){
				strcat(data, "[");

				for(int k = 0; k < sensor_data[0].valuesLen; k++){
					memset(&aux, 0, CHAR_LENGTH);
					if (sensor_data[j].sensor_values[k].sensor_value_type == INTEGER){
						sprintf(aux, "%d", sensor_data[j].sensor_values[k].sensor_value.ival);
					}else if (sensor_data[j].sensor_values[k].sensor_value_type == FLOAT){
						sprintf(aux, "%f", sensor_data[j].sensor_values[k].sensor_value.fval);
					}else {
						strcpy(aux, "\"");
						strcat(aux, sensor_data[j].sensor_values[k].sensor_value.cval);
						strcat(aux, "\"");
					}
					strcat(data, aux);

					if(k < sensor_data[0].valuesLen - 1){
						strcat(data, ",");
					}
				}

				strcat(data, "]");

				if(j < number_of_sensors - 1){
					strcat(data, ",");
				}
			}

			strcat(data, "]");

			if(first)
				first = false;
		}

		// Free memory
		if(sensor_data != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_data[j].sensor_values);
			}
			free(sensor_data);
		}
	}

	strcat(data, "}");
}

void get_sensors_gpios_json(char *data){

	int number_of_sensors;
	sensor_gpios_info_t* sensor_gpios;
	bool first = true;

	char aux[CHAR_LENGTH];

	strcpy(data, "{");

	for(int i = 0; i < recollecters_n; i++){

		sensor_gpios = get_sensor_gpios(i, &number_of_sensors);

		if(sensor_gpios != NULL){

			if(!first){
				strcat(data, ",");
			}

			strcat(data, "\"");
			memset(&aux, 0, CHAR_LENGTH);
			sprintf(aux, "%d", i);
			strcat(data, aux);
			strcat(data, "\":[");

			for(int j = 0; j < number_of_sensors; j++){
				strcat(data, "[");

				for(int k = 0; k < sensor_gpios[0].gpiosLen; k++){
					memset(&aux, 0, CHAR_LENGTH);
					sprintf(aux, "%d", sensor_gpios[j].sensor_gpios[k].sensor_gpio);
					strcat(data, aux);

					if(k < sensor_gpios[0].gpiosLen - 1){
						strcat(data, ",");
					}
				}

				strcat(data, "]");

				if(j < number_of_sensors - 1){
					strcat(data, ",");
				}
			}

			strcat(data, "]");

			if(first)
				first = false;
		}

		// Free memory
		if(sensor_gpios != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_gpios[j].sensor_gpios);
			}
			free(sensor_gpios);
		}
	}

	strcat(data, "}");
}

void get_sensors_parameters_json(char *data){

	int number_of_sensors;
	sensor_additional_parameters_info_t* sensor_parameters;
	bool first = true;

	char aux[CHAR_LENGTH];

	strcpy(data, "{");

	for(int i = 0; i < recollecters_n; i++){

		sensor_parameters = get_sensor_parameters(i, &number_of_sensors);

		if(sensor_parameters != NULL){

			if(!first){
				strcat(data, ",");
			}

			strcat(data, "\"");
			memset(&aux, 0, CHAR_LENGTH);
			sprintf(aux, "%d", i);
			strcat(data, aux);
			strcat(data, "\":[");

			for(int j = 0; j < number_of_sensors; j++){
				strcat(data, "[");

				for(int k = 0; k < sensor_parameters[0].parametersLen; k++){
					memset(&aux, 0, CHAR_LENGTH);
					if (sensor_parameters[j].sensor_parameters[k].sensor_parameter_type == INTEGER){
						sprintf(aux, "%d", sensor_parameters[j].sensor_parameters[k].sensor_parameter.ival);
					}else if (sensor_parameters[j].sensor_parameters[k].sensor_parameter_type == FLOAT){
						sprintf(aux, "%f", sensor_parameters[j].sensor_parameters[k].sensor_parameter.fval);
					}else {
						strcpy(aux, "\"");
						strcat(aux, sensor_parameters[j].sensor_parameters[k].sensor_parameter.cval);
						strcat(aux, "\"");
					}
					strcat(data, aux);

					if(k < sensor_parameters[0].parametersLen - 1){
						strcat(data, ",");
					}
				}

				strcat(data, "]");

				if(j < number_of_sensors - 1){
					strcat(data, ",");
				}
			}

			strcat(data, "]");

			if(first)
				first = false;
		}

		// Free memory
		if(sensor_parameters != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_parameters[j].sensor_parameters);
			}
			free(sensor_parameters);
		}
	}

	strcat(data, "}");
}

void get_sensors_alerts_json(char *data){

	int number_of_sensors;
	sensor_data_t* sensor_data;
	bool first = true;

	char aux[CHAR_LENGTH];

	strcpy(data, "{");

	for(int i = 0; i < recollecters_n; i++){

		sensor_data = get_sensor_data(i, &number_of_sensors);

		if(sensor_data != NULL){

			if(!first){
				strcat(data, ",");
			}

			strcat(data, "\"");
			memset(&aux, 0, CHAR_LENGTH);
			sprintf(aux, "%d", i);
			strcat(data, aux);
			strcat(data, "\":[");

			for(int j = 0; j < sensor_data[0].valuesLen; j++){
				strcat(data, "[");

				if(sensor_data[0].sensor_values[j].alert){
					strcat(data, "true");
				}
				else{
					strcat(data, "false");
				}

				strcat(data, ",");

				memset(&aux, 0, CHAR_LENGTH);
				sprintf(aux, "%d", sensor_data[0].sensor_values[j].ticks_to_alert);
				strcat(data, aux);

				strcat(data, ",");

				if(sensor_data[0].sensor_values[j].sensor_value_type == INTEGER){
					memset(&aux, 0, CHAR_LENGTH);
					sprintf(aux, "%d", sensor_data[0].sensor_values[j].upper_threshold.ival);
					strcat(data, aux);

					strcat(data, ",");

					memset(&aux, 0, CHAR_LENGTH);
					sprintf(aux, "%d", sensor_data[0].sensor_values[j].lower_threshold.ival);
					strcat(data, aux);
				}
				else if(sensor_data[0].sensor_values[j].sensor_value_type == FLOAT){
					memset(&aux, 0, CHAR_LENGTH);
					sprintf(aux, "%f", sensor_data[0].sensor_values[j].upper_threshold.fval);
					strcat(data, aux);

					strcat(data, ",");

					memset(&aux, 0, CHAR_LENGTH);
					sprintf(aux, "%f", sensor_data[0].sensor_values[j].lower_threshold.fval);
					strcat(data, aux);
				}
				else{
					memset(&aux, 0, CHAR_LENGTH);
					strcpy(aux, "\"");
					strcat(aux, sensor_data[0].sensor_values[j].upper_threshold.cval);
					strcat(aux, "\"");
					strcat(data, aux);

					strcat(data, ",");

					memset(&aux, 0, CHAR_LENGTH);
					strcpy(aux, "\"");
					strcat(aux, sensor_data[0].sensor_values[j].lower_threshold.cval);
					strcat(aux, "\"");
					strcat(data, aux);
				}

				strcat(data, "]");
				if(j < sensor_data[0].valuesLen - 1){
					strcat(data, ",");
				}
			}

			strcat(data, "]");

			if(first)
				first = false;
		}

		// Free memory
		if(sensor_data != NULL){
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_data[j].sensor_values);
			}
			free(sensor_data);
		}
	}

	strcat(data, "}");
}

int get_sensor_data_cjson (int sensor_id, int pos, char* data, char* sensorName){
	char aux[CHAR_LENGTH], *printAux;
	sensor_data_t* all;
	sensor_data_t sensor_data;
	int i;
	int number_of_sensors;
	struct cJSON *jobj;

	all = get_sensor_data(sensor_id, &number_of_sensors);

	if (all == NULL) return -1;

	if(pos < 0 || pos >= number_of_sensors) return -1;

	sensor_data = all[pos];

	strcpy(sensorName, sensor_data.sensorName);

	jobj = cJSON_CreateObject();

	for(i = 0;i < sensor_data.valuesLen;i++){
		memset(&aux,0,CHAR_LENGTH);
		itoa(i+1,aux,10);
		if (sensor_data.sensor_values[i].sensor_value_type == INTEGER){
			cJSON_AddNumberToObject(jobj,aux,sensor_data.sensor_values[i].sensor_value.ival);
		}else if (sensor_data.sensor_values[i].sensor_value_type == FLOAT){
			cJSON_AddNumberToObject(jobj,aux,sensor_data.sensor_values[i].sensor_value.fval);
		}else {
			cJSON_AddStringToObject(jobj,aux,sensor_data.sensor_values[i].sensor_value.cval);
		}
	}


	printAux = cJSON_Print(jobj);

	strcpy(data,printAux);

	free(printAux);

	return strlen(data);
}

int get_sensor_data_json (int sensor_id, int pos, char* data, char* sensorName){
	char aux[CHAR_LENGTH];
	sensor_data_t* all;
	sensor_data_t sensor_data;
	int i;
	int number_of_sensors;
	int len = 0;

	all = get_sensor_data(sensor_id, &number_of_sensors);

	if (all == NULL) return -1;

	if(pos < 0 || pos >= number_of_sensors) return -1;

	sensor_data = all[pos];

	strcpy(sensorName, sensor_data.sensorName);

	strcpy(data, "{");
	for (i = 0; i < sensor_data.valuesLen;i++){
		if(i > 0){
			strcat(data,",");
		}
		memset(&aux,0,CHAR_LENGTH);
		sprintf(aux, "%d",i + 1);
		strcat(data,"\"");
		strcat(data, aux);
		strcat(data,"\":");

		memset(&aux,0,CHAR_LENGTH);
		if (sensor_data.sensor_values[i].sensor_value_type == INTEGER){
			sprintf(aux, "%d",sensor_data.sensor_values[i].sensor_value.ival);
		}else if (sensor_data.sensor_values[i].sensor_value_type == FLOAT){
			sprintf(aux, "%f",sensor_data.sensor_values[i].sensor_value.fval);
		}else {
			strcpy(aux, "\"");
			strcat(aux,sensor_data.sensor_values[i].sensor_value.cval );
			strcat(aux, "\"");
		}
		strcat(data, aux);
	}
	strcat(data, "}");

	len = strlen(data);

	free(all);

	return len;
}
/*
void get_sensor_data_name(int sensor_id, char *name){
	sensor_data_t sensor_data;

	sensor_data = get_sensor_data(sensor_id);

	if (sensor_data.valuesLen == 0) return;

	strcpy(name, sensor_data.sensorName);
}
*/
sensor_data_t* get_sensor_data (int sensor_id, int* number_of_sensors){
	recollecter_function foo;
	sensor_data_t* sensor_data = NULL;

	pthread_mutex_lock(&mutex_RECOLLECTER);
		if (sensor_id >= recollecters_n || sensor_id < 0){
			ESP_LOGE(TAG, "ERROR in get_sensor_data, the sensor_id is out of range!");
			return sensor_data;
		}else {
			foo = recollecters[sensor_id];
		}
	pthread_mutex_unlock(&mutex_RECOLLECTER);

	sensor_data = foo(number_of_sensors);

	return sensor_data;
}

sensor_gpios_info_t* get_sensor_gpios (int sensor_id, int* number_of_sensors){
	recollecter_gpios_function foo;
	sensor_gpios_info_t* sensor_gpios = NULL;

	pthread_mutex_lock(&mutex_RECOLLECTER);
		if (sensor_id >= recollecters_n || sensor_id < 0){
			ESP_LOGE(TAG, "ERROR in get_sensor_gpios, the sensor_id is out of range!");
			return sensor_gpios;
		}else {
			foo = recollecters_gpios[sensor_id];
		}
	pthread_mutex_unlock(&mutex_RECOLLECTER);

	sensor_gpios = foo(number_of_sensors);

	return sensor_gpios;
}

sensor_additional_parameters_info_t* get_sensor_parameters (int sensor_id, int* number_of_sensors){
	recollecter_parameters_function foo;
	sensor_additional_parameters_info_t* sensor_parameters = NULL;

	pthread_mutex_lock(&mutex_RECOLLECTER);
		if (sensor_id >= recollecters_n || sensor_id < 0){
			ESP_LOGE(TAG, "ERROR in get_sensor_parameters, the sensor_id is out of range!");
			return sensor_parameters;
		}else {
			foo = recollecters_parameters[sensor_id];
		}
	pthread_mutex_unlock(&mutex_RECOLLECTER);

	sensor_parameters = foo(number_of_sensors);

	return sensor_parameters;
}

void recollecter_start(void){
	if(pthread_mutex_init (&mutex_RECOLLECTER, NULL) != 0){
	 ESP_LOGE(TAG,"Failed to initialize the recollecter mutex");
	}

    recollecters = (recollecter_function *)malloc(sizeof(recollecter_function) * RECOLLECTER_SIZE);
    recollecters_gpios = (recollecter_gpios_function *)malloc(sizeof(recollecter_gpios_function) * RECOLLECTER_SIZE);
    recollecters_parameters = (recollecter_parameters_function *)malloc(sizeof(recollecter_parameters_function) * RECOLLECTER_SIZE);
    recollecters_n = 0;

    ESP_LOGI(TAG, "recollecter started");
}
