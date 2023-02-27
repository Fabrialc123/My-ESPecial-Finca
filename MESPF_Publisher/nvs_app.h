/*
 * nvs_app.h
 *
 *  Created on: 26 feb. 2023
 *      Author: fabri
 */

#ifndef MAIN_NVS_APP_H_
#define MAIN_NVS_APP_H_

#include <stdint.h>
#include <stddef.h>


void nvs_app_init();

void nvs_app_get_uint8_value(const char *key, uint8_t *value);
void nvs_app_set_uint8_value(const char *key, const uint8_t value);

void nvs_app_get_uint32_value(const char *key, uint32_t *value);
void nvs_app_set_uint32_value(const char *key, const uint32_t value);

void nvs_app_get_string_value(const char *key, char *value, size_t *size);
void nvs_app_set_string_value(const char *key, const char *value);

void nvs_app_get_blob_value(const char *key, void *value, size_t *size);
void nvs_app_set_blob_value(const char *key, const void *value, const size_t size);



#endif /* MAIN_NVS_APP_H_ */
