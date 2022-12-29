/*
 * wifi_app.h
 *
 *  Created on: 14 jun. 2022
 *      Author: fabri
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"

#define WIFI_STA_SSID				"Red Alcaraz"
#define WIFI_STA_PASSWORD			"SanLorenzo2019"
/*
#define WIFI_AP_CHANNEL				1				// For more details see the bandwidth of WiFi Channels and the ESP32 documentation
#define	WIFI_AP_SSID_HIDDEN			0
#define WIFI_AP_MAX_CONNECTIONS		5				// Max clients
#define	WIFI_AP_BEACON_INTERVAL		100				// Must be 100-6000. It defines the frequency of advertisement
#define WIFI_AP_IP					"192.168.0.1"
#define WIFI_AP_GATEWAY				"192.168.0.1"	// Should be the same as WIFI_AP_IP
#define WIFI_AP_NETMASK				"255.255.255.0"
#define WIFI_AP_BANDWIDTH			WIFI_BW_HT20	// 20Mhz  Bandwidth, can be also 40Mhz
#define WIFI_STA_POWER_SAVE			WIFI_PS_NONE	// See Station Sleep reference at docs
#define	MAX_SSID_LENGTH				32				// 32 is the IEEE standard maximum
#define MAX_PASSWORD_LENGTH			64				// 64 is the IEEE standard maximum
*/

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


void wifi_app_getIP(char *ip);

#endif /* MAIN_WIFI_APP_H_ */
