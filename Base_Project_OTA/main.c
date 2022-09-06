/**
 * Application entry point
 *
 * Open a Terminal and see all the events at connecting and disconnecting devices.
 */

#include "nvs_flash.h"

#include "wifi_app.h"
#include "http_server.h"

void app_main(void)
{
	// Initialize Non Volatile Storage (NVS)
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	wifi_app_start();

}

