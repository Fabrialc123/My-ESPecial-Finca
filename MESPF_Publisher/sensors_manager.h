/*
 * sensors_manager.h
 *
 *  Created on: 12 mar. 2023
 *      Author: Kike
 */

#ifndef MAIN_SENSORS_MANAGER_H_
#define MAIN_SENSORS_MANAGER_H_

#include "recollecter.h"

// Generic functions to manage sensor drivers

typedef int (*add_unit_function)(int*,union sensor_value_u*,char*);
typedef int (*delete_unit_function)(int,char*);

typedef int (*set_gpios_function)(int,int*,char*);
typedef int (*set_parameters_function)(int,union sensor_value_u*,char*);
typedef int (*set_location_function)(int,char*,char*);
typedef int (*set_alert_values_function)(int,bool,int,union sensor_value_u,union sensor_value_u,char*);

/**
 * Initialize the sensors manager
 */
void sensors_manager_init(void);

/**
 * Register new sensor to be managed
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_add(add_unit_function aun, delete_unit_function dun, set_gpios_function sgp, set_parameters_function spa, set_location_function slo, set_alert_values_function sal);

/**
 * Initialize all sensor drivers located in "/gpios" directory
 */
void sensors_manager_sensors_initialization(void);

/**
 * Add new sensor unit with identifier "id"
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_add_sensor_unit(int id, int* gpios, union sensor_value_u* parameters, char* reason);

/**
 * Delete sensor unit with identifier "id" and position "pos"
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_delete_sensor_unit(int id, int pos, char* reason);

/**
 * Set new GPIOS to sensor unit with identifier "id" and position "pos"
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_set_gpios(int id, int pos, int* gpios, char* reason);

/**
 * Set new parameters to sensor unit with identifier "id" and position "pos"
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_set_parameters(int id, int pos, union sensor_value_u* parameters, char* reason);

/**
 * Set new location to sensor unit with identifier "id" and position "pos"
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_set_location(int id, int pos, char* location, char* reason);

/**
 * Set new alert values to sensor with identifier "id"
 *
 * return 1 if done, -1 if not
 */
int sensors_manager_set_alert_values(int id, int value, bool alert, int ticks, union sensor_value_u upperThreshold, union sensor_value_u lowerThreshold, char* reason);

#endif /* MAIN_SENSORS_MANAGER_H_ */
