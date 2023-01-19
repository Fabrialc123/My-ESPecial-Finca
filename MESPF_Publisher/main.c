/**
 * Application entry point
 *
 * Open a Terminal and see all the events at connecting and disconnecting devices.
 */


#include <mqtt/mqtt_app.h>
#include "nvs_flash.h"

#include "wifi_app.h"
#include "http_server.h"
#include "esp_log.h"
#include "recollecter.h"
#include "status.h"

#include "time.h"

#include "tests.c"
#include "test_sntp.c"


static const char main_TAG[] = "MAIN";


void app_main(void)
{
	// Initialize Non Volatile Storage (NVS)
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

    ESP_LOGI(main_TAG, "[APP] Startup..");
    ESP_LOGI(main_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(main_TAG, "[APP] IDF version: %s", esp_get_idf_version());


	wifi_app_start();

	recollecter_start();

	status_start();

	mqtt_app_start();
}

