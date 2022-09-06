/*
 * wifi_app.c
 *
 *  Created on: 14 jun. 2022
 *      Author: fabri
 */

#include <gpios/led_rgb.h>
#include "wifi_app.h"
#include "task_common.h"
#include "http_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"

static const char TAG[] = "wifi_app";

static QueueHandle_t wifi_app_queue_handle;

esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap = NULL;

int s_retry_num = 0;


/**
 * WiFi application event handler
 * @param arg data, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id of the event to register the handler for
 * @param event_data event data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

	if (event_base == WIFI_EVENT){
		switch(event_id){
			case WIFI_EVENT_STA_START:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
				esp_wifi_connect();
				break;

			case WIFI_EVENT_STA_CONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
				break;

			case WIFI_EVENT_STA_DISCONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
				while (s_retry_num < MAX_CONNECTION_RETRIES){
					s_retry_num++;
					ESP_LOGI(TAG," Retry nº %i to connect to the AP ", s_retry_num);
					esp_wifi_connect();
				}

				if (s_retry_num >= MAX_CONNECTION_RETRIES) ESP_LOGI(TAG, "FAILED TO CONNECT TO THE AP %s and PSSWD: %s",WIFI_STA_SSID, WIFI_STA_PASSWORD);
				break;

			case WIFI_EVENT_STA_STOP:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
				break;
		}
	}else if (event_base == IP_EVENT){
		switch(event_id){
			case IP_EVENT_STA_GOT_IP:
				ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
				break;
		}
	}

}

static void wifi_app_event_handler_init(void){
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));

}

static void wifi_app_default_wifi_init(void){
	ESP_ERROR_CHECK(esp_netif_init());

	// OPERATIONS MUST BE IN THIS ORDER!
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	esp_netif_sta = esp_netif_create_default_wifi_sta();
}

static void wifi_app_sta_config(void){
	wifi_config_t sta_config = {
		.sta = {
			.ssid = WIFI_STA_SSID,
			//.ssid_len = strlen(WIFI_AP_SSID),
			.password = WIFI_STA_PASSWORD,
		},
	};


	esp_netif_dhcps_stop(esp_netif_ap);		// Must call this first!

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA,&sta_config));
}

static void wifi_app_task (void *pvParameters){
	wifi_app_queue_msg_t msg;

	wifi_app_event_handler_init();
	wifi_app_default_wifi_init();
	wifi_app_sta_config();

	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

	for(;;){
		if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY)){
			switch(msg.msgID){
				case WIFI_APP_MSG_START_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");

					http_server_start();
					led_rgb_http_server_started();
					break;

				case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
					break;

				case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
					ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");

					led_rgb_wifi_connected();
					break;

				default:
					break;
			}
		}
	}
}

BaseType_t wifi_app_send_message(wifi_app_msg_e msgID){
	wifi_app_queue_msg_t msg;

	msg.msgID = msgID;

	return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}


void wifi_app_start(void){
	ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

	led_rgb_wifi_app_started();

	esp_log_level_set("wifi", ESP_LOG_NONE);		// Disables default WiFi log messages

	wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_msg_t));

	xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);

}


