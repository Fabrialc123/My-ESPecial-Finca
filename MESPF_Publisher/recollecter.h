/*
 * recollecter.h
 *
 *  Created on: 27 sept. 2022
 *      Author: fabri
 */

#ifndef MAIN_RECOLLECTER_H_
#define MAIN_RECOLLECTER_H_

#include "esp_netif.h"

#define RECOLLECTER_SIZE 5
#define CHAR_LENGTH 20
#define MAX_STRING_LENGTH 255

typedef enum{
	INTEGER = 0,
	FLOAT,
	STRING
} sensor_value_type_e;

union sensor_value_u{
	int ival;
	float fval;
	char cval[CHAR_LENGTH];
} ;

// --------------------------------------------------- Values -----------------------------------------------------------------

typedef struct {
	bool showOnLCD;
	char valueName[CHAR_LENGTH];
	sensor_value_type_e sensor_value_type;
	union sensor_value_u sensor_value;
	bool alert;
	int ticks_to_alert;
	union sensor_value_u upper_threshold;
	union sensor_value_u lower_threshold;
}sensor_value_t;

typedef struct {
	char sensorName[CHAR_LENGTH];
	char sensorLocation[CHAR_LENGTH+1];
	int valuesLen;
	sensor_value_t *sensor_values;
}sensor_data_t;

// ---------------------------------------------------- GPIOS -----------------------------------------------------------------

typedef struct{
	char gpioName[CHAR_LENGTH];
	int sensor_gpio;
}sensor_gpio_t;

typedef struct{
	int gpiosLen;
	sensor_gpio_t *sensor_gpios;
}sensor_gpios_info_t;

// ------------------------------------------- Additional parameters ----------------------------------------------------------

typedef struct{
	char parameterName[CHAR_LENGTH];
	sensor_value_type_e sensor_parameter_type;
	union sensor_value_u sensor_parameter;
}sensor_additional_parameter_t;

typedef struct{
	int parametersLen;
	sensor_additional_parameter_t *sensor_parameters;
}sensor_additional_parameters_info_t;

// ----------------------------------------------------------------------------------------------------------------------------

typedef sensor_data_t* (*recollecter_function)(int*);
typedef sensor_gpios_info_t* (*recollecter_gpios_function)(int*);
typedef sensor_additional_parameters_info_t* (*recollecter_parameters_function)(int*);

int register_recollecter (recollecter_function rtr, recollecter_gpios_function rtg, recollecter_parameters_function rtp);
int delete_recollecter (int id);

int get_recollecters_size (void);

void get_sensors_configuration_cjson(char *data);
void get_sensors_locations_cjson(char *data);
void get_sensors_values_cjson(char *data);
void get_sensors_gpios_cjson(char *data);
void get_sensors_parameters_cjson(char *data);
void get_sensors_alerts_cjson(char *data);

//int get_sensor_data_json (int sensor_id, int pos, char *data, char *sensorName);
int get_sensor_data_cjson (int sensor_id, int pos, char *data, char *sensorName);

int get_sensor_id_by_name(char *sensor_name);

//void get_sensor_data_name(int sensor_id, char *name);

sensor_data_t* get_sensor_data (int sensor_id, int* number_of_sensors);
sensor_gpios_info_t* get_sensor_gpios (int sensor_id, int* number_of_sensors);
sensor_additional_parameters_info_t* get_sensor_parameters (int sensor_id, int* number_of_sensors);

void recollecter_start(void);



#endif /* MAIN_RECOLLECTER_H_ */
