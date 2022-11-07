/*
 * wifi_app.h
 *
 *  Created on: 14 jun. 2022
 *      Author: fabri
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"

//#define WIFI_STA_SSID				""
//#define WIFI_STA_PASSWORD			""

extern esp_netif_t*	esp_netif_sta;
extern esp_netif_t*	esp_netif_ap;

/**
 * Messages IDs for the WiFi application task
 * @note Can be expanded for application requirements
 */
typedef enum{
	WIFI_APP_MSG_START_HTTP_SERVER = 0,
	WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP
} wifi_app_msg_e;

/**
 * Structure for the message queue
 * @note Can be expanded for application requirements
 */
typedef struct{
	wifi_app_msg_e	msgID;
} wifi_app_queue_msg_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the wifi_app_msg_e enum
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE
 */
BaseType_t wifi_app_send_message(wifi_app_msg_e msgID);

/**
 * Starts the WiFi application
 */
void wifi_app_start(void);

#endif /* MAIN_WIFI_APP_H_ */
