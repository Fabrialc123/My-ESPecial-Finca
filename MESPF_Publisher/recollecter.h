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

typedef struct {
	bool showOnLCD;
	char valueName[CHAR_LENGTH];
	sensor_value_type_e sensor_value_type;
	union sensor_value_u sensor_value;
}sensor_value_t;

typedef struct {
	char sensorName[CHAR_LENGTH];
	int valuesLen;
	sensor_value_t *sensor_values;
}sensor_data_t;


typedef sensor_data_t (*recollecter_function)(void);

int register_recollecter (recollecter_function rtr);

int get_recollecters_size (void);

int get_sensor_data_json (int sensor_id, char *data, char *sensorName);

//void get_sensor_data_name(int sensor_id, char *name);

sensor_data_t get_sensor_data (int sensor_id);

void recollecter_start(void);



#endif /* MAIN_RECOLLECTER_H_ */
