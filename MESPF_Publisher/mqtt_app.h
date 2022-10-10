/*
 * mqtt_app.h
 *
 *  Created on: 25 sept. 2022
 *      Author: fabri
 */

#ifndef MAIN_MQTT_APP_H_
#define MAIN_MQTT_APP_H_

#include "esp_netif.h"
#include "recollecter.h"
//#include "mqtt_app.c"

#define MQTT_APP_TIME_TO_SEND_DATA			100 * 30 // 1 second = 100 ticks
#define MQTT_APP_COMMANDS_TOPIC 			"COMMANDS"
#define MQTT_APP_REFRESH_TOPIC				"COMMANDS/REFRESH"
#define MQTT_APP_SCAN_TOPIC				"COMMANDS/SCAN"
#define MQTT_APP_SCAN_RESP_TOPIC			"/SCANRESP"
#define MQTT_APP_GENERAL_TOPIC 				"FINCA_TOPIC"
//#define MQTT_APP_PERSONAL_TOPIC				"ESP32_%CHIPID%"
#define MQTT_APP_QOS						0

#define MQTT_APP_PORT 						1883
//#define MQTT_APP_HOST 						"192.168.68.78"
#define MQTT_APP_URI 						"mqtt://broker.hivemq.com"
//#define MQTT_APP_URI 						"mqtt://mqtt.eclipseprojects.io"
//#define MQTT_APP_URI 						"mqtt://iot.eclipse.org"

#define MQTT_APP_TOPIC_LENGTH	128
#define MQTT_APP_MAX_CONNECTION_RETRIES 3


/**
 * Messages IDs for the MQTT application task
 * @note Can be expanded for application requirements
 */
typedef enum{
	MQTT_APP_MSG_SEND_DATA = 0,
	MQTT_APP_MSG_SUBSCRIBE,
	MQTT_APP_MSG_SCAN_RESPONSE,
	MQTT_APP_MSG_DISCONNECT
} mqtt_app_msg_e;

/**
 * Structure for the message queue
 * @note Can be expanded for application requirements
 */
typedef struct{
	mqtt_app_msg_e	msgID;
	char	desc[MQTT_APP_TOPIC_LENGTH];
} mqtt_app_queue_msg_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the mqtt_app_msg_e enum
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE
 */
BaseType_t mqtt_app_send_message(mqtt_app_msg_e msgID, char desc[MQTT_APP_TOPIC_LENGTH]);

/**
 * Starts the MQTT application
 */
void mqtt_app_start(void);

void mqtt_app_refresh_TEST(void);

#endif /* MAIN_MQTT_APP_H_ */
