/*
 * mqtt_app.h
 *
 *  Created on: 25 sept. 2022
 *      Author: fabri
 */

#ifndef MAIN_MQTT_MQTT_APP_H_
#define MAIN_MQTT_MQTT_APP_H_

#include "esp_netif.h"
#include "recollecter.h"
//#include "mqtt_app.c"

#define MQTT_APP_TIME_TO_SEND_DATA			100 * 300 // 1 second = 100 ticks
#define MQTT_APP_MLSECS_TO_RECONNECT 		1000 * 60
#define MQTT_APP_QOS						0

#define MQTT_APP_PORT 						1883
#define MQTT_APP_HOST 						"192.168.0.10"
//#define MQTT_APP_URI 						"mqtt://broker.hivemq.com"
//#define MQTT_APP_URI 						"mqtt://mqtt.eclipseprojects.io"
//#define MQTT_APP_URI 						"mqtt://iot.eclipse.org"

#define MQTT_APP_MAX_TOPIC_LENGTH	128
#define MQTT_APP_MAX_DATA_LENGTH	255
#define MQTT_APP_QUEUE_HANDLE_SIZE	10

#define MQTT_USER "MESPF_USER"
#define	MQTT_PASSWD "MESPF_USER"




/**
 * Messages IDs for the MQTT application task
 * @note Can be expanded for application requirements
 */
typedef enum{
	MQTT_APP_MSG_PUBLISH_DATA = 0,
	MQTT_APP_MSG_SUBSCRIBE,
	MQTT_APP_MSG_DISCONNECT,
	MQTT_APP_MSG_PROCESS_COMMAND

} mqtt_app_msg_e;

/**
 * Structure for the message queue
 * @note Can be expanded for application requirements
 */
typedef struct{
	mqtt_app_msg_e	msgID;
	char	src[MQTT_APP_MAX_TOPIC_LENGTH];
	char	data[MQTT_APP_MAX_DATA_LENGTH];
} mqtt_app_queue_msg_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the mqtt_app_msg_e enum
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE
 */
BaseType_t mqtt_app_send_message(mqtt_app_msg_e msgID, char src[MQTT_APP_MAX_TOPIC_LENGTH],char data[MQTT_APP_MAX_DATA_LENGTH]);

/**
 * Starts the MQTT application
 */
void mqtt_app_start(void);

void mqtt_app_getID(char *id);

#endif /* MAIN_MQTT_MQTT_APP_H_ */
