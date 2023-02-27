/**
 * Application entry point
 *
 * Open a Terminal and see all the events at connecting and disconnecting devices.
 */


#include <mqtt/mqtt_app.h>

#include <nvs_app.h>
#include <wifi_app.h>
#include <http_server.h>
#include <esp_log.h>
#include <recollecter.h>
#include <status.h>


static const char main_TAG[] = "MAIN";


void app_main(void)
{

	nvs_app_init();
    ESP_LOGI(main_TAG, "[APP] Startup..");
    ESP_LOGI(main_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(main_TAG, "[APP] IDF version: %s", esp_get_idf_version());

	wifi_app_start();

	recollecter_start();

	status_start();

	mqtt_app_start();
}

