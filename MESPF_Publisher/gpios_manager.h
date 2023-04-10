/*
 * gpios.h
 *
 *  Created on: 28 feb. 2023
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_MANAGER_H_
#define MAIN_GPIOS_MANAGER_H_

#include <stdbool.h>

/**
 * Initialize the GPIOS manager
 */
void gpios_manager_init(void);

/**
 * Lock the GPIOS and remove them from the list
 *
 * return 1 if done, -1 if not
 */
int gpios_manager_lock_gpios(int* gpios, int size, char* reason);

/**
 * Free the GPIOS and add them to the list
 *
 * return 1 if done, -1 if not
 */
int gpios_manager_free_gpios(int* gpios, int size, char* reason);

/**
 * Get the GPIOS available
 *
 * return array with all free GPIOS, the size is given in parameter "number_of_gpios". Or NULL if something wrong happened
 */
int* gpios_manager_get_gpios_availables(int* number_of_gpios);

#endif /* MAIN_GPIOS_MANAGER_H_ */
