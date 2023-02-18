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

#include <pthread.h>

#include <tcpip_adapter.h> // TO GET IP

#include <log.h>

static const char TAG[] = "wifi_app";

static QueueHandle_t wifi_app_queue_handle;

esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap = NULL;

pthread_mutex_t mutex_WIFI;

static int WIFI_CONNECTED = 0;
static int s_retry_num = 0;


/**
 * WiFi application event handler
 * @param arg data, aside from event data, that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id of the event to register the handler for
 * @param event_data event data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
	char dis_log[LOG_MSG_LEN];
	wifi_event_sta_disconnected_t *data;
	if (event_base == WIFI_EVENT){
		switch(event_id){
			case WIFI_EVENT_STA_START:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
				log_add("WIFI_EVENT_STA_START\n");
				led_rgb_wifi_app_started(); // WHITE
				esp_wifi_connect();
				break;

			case WIFI_EVENT_STA_CONNECTED:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
				log_add("WIFI_EVENT_STA_CONNECTED\n");
				led_rgb_http_server_started(); 	// YELLOW

				s_retry_num = 0;

				break;

			case WIFI_EVENT_STA_DISCONNECTED:

				data = (wifi_event_sta_disconnected_t*) event_data;
				ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

				sprintf(dis_log,"WIFI_EVENT_STA_DISCONNECTED, Reason: %d \n", data->reason);
				log_add(dis_log);

				pthread_mutex_lock(&mutex_WIFI);
					WIFI_CONNECTED = 0;
					led_rgb_test(); // RED
				pthread_mutex_unlock(&mutex_WIFI);

				if (s_retry_num < WIFI_MAX_CONN_RETRIES){
					s_retry_num++;
					ESP_LOGI(TAG," Retry %i to connect to the AP ", s_retry_num);
					esp_wifi_connect();
				} else if (s_retry_num == WIFI_MAX_CONN_RETRIES){
					s_retry_num++;
					esp_wifi_stop();
				}
				break;

			case WIFI_EVENT_STA_STOP:
				ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
				log_add("WIFI STOP! Restarting...");
				wifi_app_send_message(WIFI_APP_MSG_RESTART);
			break;
		}
	}else if (event_base == IP_EVENT){
		switch(event_id){
			case IP_EVENT_STA_GOT_IP:
				ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
				pthread_mutex_lock(&mutex_WIFI);
					WIFI_CONNECTED = 1;
					led_rgb_wifi_connected();	// GREEN
				pthread_mutex_unlock(&mutex_WIFI);
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
	esp_netif_ap = esp_netif_create_default_wifi_ap();

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
}

static void wifi_app_sta_config(void){
	wifi_config_t sta_config = {
		.sta = {
			.ssid = WIFI_STA_SSID,
			//.ssid_len = strlen(WIFI_AP_SSID),
			.password = WIFI_STA_PASSWORD,
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA,&sta_config));
}

static void wifi_app_soft_ap_config(void){
	uint8_t mac[6];
	char WIFI_AP_SSID[32];
	char dis_log[LOG_MSG_LEN];
	int i;

    wifi_config_t ap_config = {
		.ap = {
	/*		.ssid = "TEST",
			.ssid_len = strlen(WIFI_AP_SSID), */
			.password = WIFI_AP_PASSWORD,
			.channel = WIFI_AP_CHANNEL,
			.ssid_hidden = WIFI_AP_SSID_HIDDEN,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.max_connection = WIFI_AP_MAX_CONNECTIONS,
			.beacon_interval = WIFI_AP_BEACON_INTERVAL,
		},
	};

    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(WIFI_AP_SSID, "ESP32_%02x%02X%02X", mac[3], mac[4], mac[5]);

    //strcpy(ap_config.ap.ssid,WIFI_AP_SSID);
    //ap_config.ap.ssid = strtoul(WIFI_AP_SSID, NULL, 32);
    for(i = 0; i < 32;i++){
    	if (i <= strlen(WIFI_AP_SSID)){
    		ap_config.ap.ssid[i] = WIFI_AP_SSID[i];
    	}
    }
    ap_config.ap.ssid_len = strlen(WIFI_AP_SSID);

    ESP_LOGI(TAG,"WIFI_AP_SSID: %s",WIFI_AP_SSID);
    sprintf(dis_log, "WIFI_AP_SSID: %s \n",WIFI_AP_SSID);
    log_add(dis_log);



	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

	esp_netif_dhcps_stop(esp_netif_ap);		// Must call this first!
	// Asign Access Point's static IP, Gateway and Netmask
	inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);
	inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);
	ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info));		//Statically configure the network interface
	ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));					// Start the AP DHCP server

	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP,&ap_config));
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_AP_BANDWIDTH));
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));					// Power save
}

static void wifi_app_task (void *pvParameters){
	wifi_app_queue_msg_t msg;

	wifi_app_event_handler_init();
	wifi_app_default_wifi_init();
	wifi_app_sta_config();
	wifi_app_soft_ap_config();

	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

	ESP_LOGE(TAG,"STACK SIZE: %d / %d",uxTaskGetStackHighWaterMark(NULL), WIFI_APP_TASK_STACK_SIZE);

	for(;;){
		if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY)){
			switch(msg.msgID){
				case WIFI_APP_MSG_START_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
					http_server_start();
					break;

				case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
					break;

				case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
					ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");
					break;

				case WIFI_APP_MSG_RESTART:
					ESP_LOGI(TAG, "WIFI_APP_MSG_RESTART");

					ESP_ERROR_CHECK(esp_wifi_deinit());
					esp_netif_destroy(esp_netif_sta);
					esp_netif_destroy(esp_netif_ap);

					wifi_app_default_wifi_init();
					wifi_app_sta_config();
					wifi_app_soft_ap_config();

					s_retry_num = 0;

					ESP_ERROR_CHECK(esp_wifi_start());

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

int is_wifi_connected(){
	int result ;

pthread_mutex_lock(&mutex_WIFI);
	result = WIFI_CONNECTED;
pthread_mutex_unlock(&mutex_WIFI);

return result;
}

void wifi_app_start(void){
	ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

	led_rgb_wifi_app_started();

	esp_log_level_set("wifi", ESP_LOG_NONE);		// Disables default WiFi log messages

	wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_msg_t));


	if(pthread_mutex_init (&mutex_WIFI, NULL) != 0){
	 ESP_LOGE(TAG,"Failed to initialize the wifi mutex");
	}

	xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);

	while(!is_wifi_connected()){
		vTaskDelay(10);
	}

}

void wifi_app_getIP(char *ip){

	tcpip_adapter_ip_info_t ipInfo;

	if (is_wifi_connected()){
		tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
		sprintf(ip, IPSTR, IP2STR(&ipInfo.ip));
	}else {
		sprintf(ip, "N/A");
	}
}




