/*
 * gpios.h
 *
 *  Created on: 28 feb. 2023
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_MANAGER_H_
#define MAIN_GPIOS_MANAGER_H_

#include <stdbool.h>

#define ORIGINAL_CONT 30

/**
 * Initialize the GPIOS manager
 */
void gpios_manager_init(void);

/**
 * Lock the GPIOS and remove them from the list
 *
 * return 1 if it is valid, -1 if not
 */
int gpios_manager_lock(int* gpios, int size);

/**
 * Free the GPIOS and add them to the list
 *
 * return 1 if it is valid, -1 if not
 */
int gpios_manager_free(int* gpios, int size);

/**
 * Look if the GPIO is free or not
 *
 * return true if free, false if not
 */
bool gpios_manager_is_free(int gpio);

/**
 * Get the number of GPIOS available
 *
 * return cont
 */
int gpios_manager_get_cont(void);

/**
 * Get the GPIOS available
 *
 * return array with size "cont" filled with all GPIOS available
 */
int* gpios_manager_get_availables(void);

/**
 * Get the GPIOS available (JSON format)
 */
void gpios_manager_json(char *data);

/**
 * Check if GPIO is ADC1
 *
 * return true if it is, false if not
 */
bool gpios_manager_check_adc1(int gpio);


#endif /* MAIN_GPIOS_MANAGER_H_ */
