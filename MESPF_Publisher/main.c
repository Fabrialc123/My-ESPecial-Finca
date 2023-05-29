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
#include <gpios_manager.h>
#include <sensors_manager.h>
#include <gpios/lcd.h>

static const char main_TAG[] = "MAIN";


void app_main(void)
{
	nvs_app_init();
    ESP_LOGI(main_TAG, "[APP] Startup..");
    ESP_LOGI(main_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(main_TAG, "[APP] IDF version: %s", esp_get_idf_version());

    wifi_app_start();
	
    recollecter_start();
    gpios_manager_init();
    sensors_manager_init();

    status_start();
    sensors_manager_sensors_initialization();

    mqtt_app_start();

    lcd_init();
}
