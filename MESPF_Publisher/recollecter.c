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

void get_sensors_configuration_cjson(char *data){
	int number_of_sensors, cont = get_recollecters_size(), n_free;
	sensor_data_t* sensor_data;
	sensor_gpios_info_t* sensor_gpios;
	sensor_additional_parameters_info_t* sensor_parameters;
	cJSON *jobj, *sensorjobj, *sensorsArray, *auxArray;

	jobj = cJSON_CreateObject();

	cJSON_AddNumberToObject(jobj, "nSensors", cont);
	sensorsArray = cJSON_AddArrayToObject(jobj, "sensors");

	for(int i = 0; i < cont; i++){

		sensorjobj = cJSON_CreateObject();

		sensor_data = get_sensor_data(i, &number_of_sensors);

		if(sensor_data != NULL){

			// Fill with sensor data information
			cJSON_AddStringToObject(sensorjobj, "sensorName", sensor_data[0].sensorName);
			cJSON_AddNumberToObject(sensorjobj, "numberOfUnits", number_of_sensors);
			cJSON_AddNumberToObject(sensorjobj, "numberOfValues", sensor_data[0].valuesLen);
			auxArray = cJSON_AddArrayToObject(sensorjobj, "valueNames");
			for(int j = 0; j < sensor_data[0].valuesLen; j++){
				cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_data[0].sensor_values[j].valueName));
			}
			auxArray = cJSON_AddArrayToObject(sensorjobj, "valueTypes");
			for(int j = 0; j < sensor_data[0].valuesLen; j++){
				if(sensor_data[0].sensor_values[j].sensor_value_type == INTEGER)
					cJSON_AddItemToArray(auxArray, cJSON_CreateString("INTEGER"));
				else if(sensor_data[0].sensor_values[j].sensor_value_type == FLOAT)
					cJSON_AddItemToArray(auxArray, cJSON_CreateString("FLOAT"));
				else
					cJSON_AddItemToArray(auxArray, cJSON_CreateString("STRING"));
			}

			// Free sensor data pointers
			n_free = 0;

			do{
				free(sensor_data[n_free].sensor_values);
				n_free++;
			}while (n_free < number_of_sensors);
			free(sensor_data);
		}
		else{
			cJSON_AddNullToObject(sensorjobj, "sensorName");
			cJSON_AddNullToObject(sensorjobj, "numberOfUnits");
			cJSON_AddNullToObject(sensorjobj, "numberOfValues");
			cJSON_AddNullToObject(sensorjobj, "valueNames");
			cJSON_AddNullToObject(sensorjobj, "valueTypes");
		}

		sensor_gpios = get_sensor_gpios(i, &number_of_sensors);

		if(sensor_gpios != NULL){

			// Fill with sensor GPIOS information
			cJSON_AddNumberToObject(sensorjobj, "numberOfGpios", sensor_gpios[0].gpiosLen);
			auxArray = cJSON_AddArrayToObject(sensorjobj, "gpioNames");
			for(int j = 0; j < sensor_gpios[0].gpiosLen; j++){
				cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_gpios[0].sensor_gpios[j].gpioName));
			}

			// Free sensor GPIOS pointers
			n_free = 0;

			do{
				free(sensor_gpios[n_free].sensor_gpios);
				n_free++;
			}while(n_free < number_of_sensors);
			free(sensor_gpios);
		}
		else{
			cJSON_AddNullToObject(sensorjobj, "numberOfGpios");
			cJSON_AddNullToObject(sensorjobj, "gpioNames");
		}

		sensor_parameters = get_sensor_parameters(i, &number_of_sensors);

		if(sensor_parameters != NULL){

			// Fill with sensor parameters information
			cJSON_AddNumberToObject(sensorjobj, "numberOfParameters", sensor_parameters[0].parametersLen);
			auxArray = cJSON_AddArrayToObject(sensorjobj, "parameterNames");
			for(int j = 0; j < sensor_parameters[0].parametersLen; j++){
				cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_parameters[0].sensor_parameters[j].parameterName));
			}
			auxArray = cJSON_AddArrayToObject(sensorjobj, "parameterTypes");
			for(int j = 0; j < sensor_parameters[0].parametersLen; j++){
				if(sensor_parameters[0].sensor_parameters[j].sensor_parameter_type == INTEGER)
					cJSON_AddItemToArray(auxArray, cJSON_CreateString("INTEGER"));
				else if(sensor_parameters[0].sensor_parameters[j].sensor_parameter_type == FLOAT)
					cJSON_AddItemToArray(auxArray, cJSON_CreateString("FLOAT"));
				else
					cJSON_AddItemToArray(auxArray, cJSON_CreateString("STRING"));
			}

			// Free sensor parameters pointers
			n_free = 0;

			do{
				free(sensor_parameters[n_free].sensor_parameters);
				n_free++;
			}while(n_free < number_of_sensors);
			free(sensor_parameters);
		}
		else{
			cJSON_AddNullToObject(sensorjobj, "numberOfParameters");
			cJSON_AddNullToObject(sensorjobj, "parameterNames");
			cJSON_AddNullToObject(sensorjobj, "parameterTypes");
		}

		cJSON_AddItemToArray(sensorsArray, sensorjobj);
	}

	strcpy(data, cJSON_Print(jobj));
	cJSON_Delete(jobj);
}

void get_sensors_locations_cjson(char *data){
	char aux[CHAR_LENGTH];
	int number_of_sensors, cont = get_recollecters_size();
	sensor_data_t* sensor_data;
	cJSON *jobj, *sensorArray;

	jobj = cJSON_CreateObject();

	for(int i = 0; i < cont; i++){

		sensor_data = get_sensor_data(i, &number_of_sensors);

		itoa(i, aux, 10);

		if(sensor_data != NULL && number_of_sensors > 0){

			sensorArray = cJSON_AddArrayToObject(jobj, aux);

			for(int j = 0; j < number_of_sensors; j++){
				cJSON_AddItemToArray(sensorArray, cJSON_CreateString(sensor_data[j].sensorLocation));
			}

			// Free memory
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_data[j].sensor_values);
			}
			free(sensor_data);
		}
		else{
			cJSON_AddNullToObject(jobj, aux);
		}
	}

	strcpy(data, cJSON_Print(jobj));
	cJSON_Delete(jobj);
}

void get_sensors_values_cjson(char *data){
	char aux[CHAR_LENGTH];
	int number_of_sensors, cont = get_recollecters_size();
	sensor_data_t* sensor_data;
	cJSON *jobj, *sensorArray, *auxArray;

	jobj = cJSON_CreateObject();

	for(int i = 0; i < cont; i++){

		sensor_data = get_sensor_data(i, &number_of_sensors);

		itoa(i, aux, 10);

		if(sensor_data != NULL && number_of_sensors > 0){

			sensorArray = cJSON_AddArrayToObject(jobj, aux);

			for(int j = 0; j < number_of_sensors; j++){

				auxArray = cJSON_CreateArray();

				for(int k = 0; k < sensor_data[0].valuesLen; k++){
					if(sensor_data[j].sensor_values[k].sensor_value_type == INTEGER)
						cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[j].sensor_values[k].sensor_value.ival));
					else if(sensor_data[j].sensor_values[k].sensor_value_type == FLOAT)
						cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[j].sensor_values[k].sensor_value.fval));
					else
						cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_data[j].sensor_values[k].sensor_value.cval));
				}

				cJSON_AddItemToArray(sensorArray, auxArray);
			}

			// Free memory
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_data[j].sensor_values);
			}
			free(sensor_data);
		}
		else{
			cJSON_AddNullToObject(jobj, aux);
		}
	}

	strcpy(data, cJSON_Print(jobj));
	cJSON_Delete(jobj);
}

void get_sensors_gpios_cjson(char *data){
	char aux[CHAR_LENGTH];
	int number_of_sensors, cont = get_recollecters_size();
	sensor_gpios_info_t* sensor_gpios;
	cJSON *jobj, *sensorArray, *auxArray;

	jobj = cJSON_CreateObject();

	for(int i = 0; i < cont; i++){

		sensor_gpios = get_sensor_gpios(i, &number_of_sensors);

		itoa(i, aux, 10);

		if(sensor_gpios != NULL && number_of_sensors > 0){

			sensorArray = cJSON_AddArrayToObject(jobj, aux);

			for(int j = 0; j < number_of_sensors; j++){

				auxArray = cJSON_CreateArray();

				for(int k = 0; k < sensor_gpios[0].gpiosLen; k++){
					cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_gpios[j].sensor_gpios[k].sensor_gpio));
				}

				cJSON_AddItemToArray(sensorArray, auxArray);
			}

			// Free memory
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_gpios[j].sensor_gpios);
			}
			free(sensor_gpios);
		}
		else{
			cJSON_AddNullToObject(jobj, aux);
		}
	}

	strcpy(data, cJSON_Print(jobj));
	cJSON_Delete(jobj);
}

void get_sensors_parameters_cjson(char *data){
	char aux[CHAR_LENGTH];
	int number_of_sensors, cont = get_recollecters_size();
	sensor_additional_parameters_info_t* sensor_parameters;
	cJSON *jobj, *sensorArray, *auxArray;

	jobj = cJSON_CreateObject();

	for(int i = 0; i < cont; i++){

		sensor_parameters = get_sensor_parameters(i, &number_of_sensors);

		itoa(i, aux, 10);

		if(sensor_parameters != NULL && number_of_sensors > 0){

			sensorArray = cJSON_AddArrayToObject(jobj, aux);

			for(int j = 0; j < number_of_sensors; j++){

				auxArray = cJSON_CreateArray();

				for(int k = 0; k < sensor_parameters[0].parametersLen; k++){
					if (sensor_parameters[j].sensor_parameters[k].sensor_parameter_type == INTEGER)
						cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_parameters[j].sensor_parameters[k].sensor_parameter.ival));
					else if (sensor_parameters[j].sensor_parameters[k].sensor_parameter_type == FLOAT)
						cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_parameters[j].sensor_parameters[k].sensor_parameter.fval));
					else
						cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_parameters[j].sensor_parameters[k].sensor_parameter.cval));
				}

				cJSON_AddItemToArray(sensorArray, auxArray);
			}

			// Free memory
			for(int j = 0; j < number_of_sensors; j++){
				free(sensor_parameters[j].sensor_parameters);
			}
			free(sensor_parameters);
		}
		else{
			cJSON_AddNullToObject(jobj, aux);
		}
	}

	strcpy(data, cJSON_Print(jobj));
	cJSON_Delete(jobj);
}

void get_sensors_alerts_cjson(char *data){
	char aux[CHAR_LENGTH];
	int number_of_sensors, cont = get_recollecters_size(), n_free;
	sensor_data_t* sensor_data;
	cJSON *jobj, *sensorArray, *auxArray;

	jobj = cJSON_CreateObject();

	for(int i = 0; i < cont; i++){

		sensor_data = get_sensor_data(i, &number_of_sensors);

		itoa(i, aux, 10);

		if(sensor_data != NULL){

			sensorArray = cJSON_AddArrayToObject(jobj, aux);

			for(int j = 0; j < sensor_data[0].valuesLen; j++){

				auxArray = cJSON_CreateArray();

				cJSON_AddItemToArray(auxArray, cJSON_CreateBool(sensor_data[0].sensor_values[j].alert));
				cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[0].sensor_values[j].ticks_to_alert));
				if(sensor_data[0].sensor_values[j].sensor_value_type == INTEGER){
					cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[0].sensor_values[j].upper_threshold.ival));
					cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[0].sensor_values[j].lower_threshold.ival));
				}
				else if(sensor_data[0].sensor_values[j].sensor_value_type == FLOAT){
					cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[0].sensor_values[j].upper_threshold.fval));
					cJSON_AddItemToArray(auxArray, cJSON_CreateNumber(sensor_data[0].sensor_values[j].lower_threshold.fval));
				}
				else{
					cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_data[0].sensor_values[j].upper_threshold.cval));
					cJSON_AddItemToArray(auxArray, cJSON_CreateString(sensor_data[0].sensor_values[j].lower_threshold.cval));
				}

				cJSON_AddItemToArray(sensorArray, auxArray);
			}

			// Free memory
			n_free = 0;

			do{
				free(sensor_data[n_free].sensor_values);
				n_free++;
			}while (n_free < number_of_sensors);
			free(sensor_data);
		}
		else{
			cJSON_AddNullToObject(jobj, aux);
		}
	}

	strcpy(data, cJSON_Print(jobj));
	cJSON_Delete(jobj);
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

int get_sensor_id_by_name(char *sensor_name){
	bool found = false;
	int id = 0, cont = get_recollecters_size(), number_of_sensors, n_free;
	sensor_data_t *sensor_data;

	while(id < cont && !found){
		sensor_data = get_sensor_data(id, &number_of_sensors);

		if(strcmp(sensor_name, sensor_data[0].sensorName) == 0)
			found = true;
		else
			id++;

		// Free pointers
		n_free = 0;

		do{
			free(sensor_data[n_free].sensor_values);
			n_free++;
		}while (n_free < number_of_sensors);
		free(sensor_data);
	}

	if(!found)
		id = -1;

	return id;
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
