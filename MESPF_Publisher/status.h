/*
 * status.h
 *
 *  Created on: 26 oct. 2022
 *      Author: fabri
 */

#ifndef MAIN_STATUS_H_
#define MAIN_STATUS_H_

#include "esp_netif.h"
#include <recollecter.h>

#define STATUS_SHOW_IP_ON_LCD		true
#define STATUS_SHOW_ID_ON_LCD		true
#define STATUS_SHOW_DATE_ON_LCD		true
#define STATUS_SHOW_UPTIME_ON_LCD	false


void status_start();

bool status_getDateTime(char *dt);
int status_setDateTime(const char *date,const char *time);

void status_ntp_get_conf(char *server, unsigned int *sync_interval, short int *status);
void status_ntp_set_conf(const char *server, const unsigned int sync_interval);

sensor_data_t* status_recollecter (int* number_of_sensors);
sensor_gpios_info_t* status_gpios_recollecter (int* number_of_sensors);
sensor_additional_parameters_info_t* status_parameters_recollecter (int* number_of_sensors);

#endif /* MAIN_STATUS_H_ */
