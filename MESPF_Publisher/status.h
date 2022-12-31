/*
 * status.h
 *
 *  Created on: 26 oct. 2022
 *      Author: fabri
 */

#ifndef MAIN_STATUS_H_
#define MAIN_STATUS_H_

#include "esp_netif.h"

void status_start();

bool status_getDateTime(char *dt);
int status_setDateTime(const char *date,const char *time);

sensor_data_t status_recollecter (void);



#endif /* MAIN_STATUS_H_ */
