/*
 * sensors_manager.h
 *
 *  Created on: 12 mar. 2023
 *      Author: Kike
 */

#ifndef MAIN_SENSORS_MANAGER_H_
#define MAIN_SENSORS_MANAGER_H_

#include "recollecter.h"

typedef void (*destroy_sensor_function)(void);
typedef int (*add_unit_function)(int*,union sensor_value_u*);
typedef int (*delete_unit_function)(int);
typedef int (*set_gpios_function)(int,int*);
typedef int (*set_parameters_function)(int,union sensor_value_u*);
typedef int (*set_alert_values_function)(int,bool,int,union sensor_value_u,union sensor_value_u);

void sensors_manager_sensors_startup(void);
void sensors_manager_validate_info(int type, int *gpios, union sensor_value_u *parameters, char *resp);

void sensors_manager_init(void);
int sensors_manager_add(destroy_sensor_function des, add_unit_function aun, delete_unit_function dun, set_gpios_function sgp, set_parameters_function spa, set_alert_values_function sal);
int sensors_manager_delete(int id);
int sensors_manager_size(void);

int sensors_manager_init_sensor(int type);
int sensors_manager_destroy_sensor(int id);
int sensors_manager_add_sensor_unit(int id, int* gpios, union sensor_value_u* parameters);
int sensors_manager_delete_sensor_unit(int id, int pos);

int sensors_manager_set_gpios(int id, int pos, int* gpios);
int sensors_manager_set_parameters(int id, int pos, union sensor_value_u* parameters);
int sensors_manager_set_alert_values(int id, int value, bool alert, int ticks, union sensor_value_u upperThreshold, union sensor_value_u lowerThreshold);

#endif /* MAIN_SENSORS_MANAGER_H_ */
