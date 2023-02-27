/*
 * nvs_app.c
 *
 *  Created on: 26 feb. 2023
 *      Author: fabri
 */

#include <nvs_app.h>

#include <nvs.h>
#include <nvs_flash.h>

#define NVS_APP_NAMESPACE "STORAGE"

void nvs_app_init(){
	// Initialize Non Volatile Storage (NVS)
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

void nvs_app_get_uint8_value(const char *key, uint8_t *value){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	nvs_get_u8(handler,key,value);
}
void nvs_app_set_uint8_value(const char *key, const uint8_t value){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	ESP_ERROR_CHECK(nvs_set_u8(handler,key,value));

	ESP_ERROR_CHECK(nvs_commit(handler));
}

void nvs_app_get_uint32_value(const char *key, uint32_t *value){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	nvs_get_u32(handler,key,value);
}
void nvs_app_set_uint32_value(const char *key, const uint32_t value){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	ESP_ERROR_CHECK(nvs_set_u32(handler,key,value));

	ESP_ERROR_CHECK(nvs_commit(handler));
}

void nvs_app_get_string_value(const char *key, char *value, size_t *size){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	nvs_get_str(handler,key,value,size);
}
void nvs_app_set_string_value(const char *key, const char *value){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	ESP_ERROR_CHECK(nvs_set_str(handler,key,value));

	ESP_ERROR_CHECK(nvs_commit(handler));
}

void nvs_app_get_blob_value(const char *key, void *value, size_t *size){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	nvs_get_blob(handler,key,value,size);
}
void nvs_app_set_blob_value(const char *key, const void *value, const size_t size){
	nvs_handle_t handler;

	ESP_ERROR_CHECK(nvs_open(NVS_APP_NAMESPACE,NVS_READWRITE,&handler));
	ESP_ERROR_CHECK(nvs_set_blob(handler,key,value,size));

	ESP_ERROR_CHECK(nvs_commit(handler));
}
