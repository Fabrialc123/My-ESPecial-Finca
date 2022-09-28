/*
 * mqtt_app.h
 *
 *  Created on: 25 sept. 2022
 *      Author: fabri
 */

#ifndef MAIN_MQTT_APP_H_
#define MAIN_MQTT_APP_H_

#include "esp_netif.h"
//#include "mqtt_app.c"

#define MQTT_APP_TIME_TO_SEND_DATA	100 * 30 // 1 second = 100 ticks
#define MQTT_APP_TOPIC "/TESTFABRI"

#define MQTT_APP_PORT 1883
#define MQTT_APP_URI "mqtt://broker.hivemq.com"
//#define MQTT_APP_URI "mqtt://mqtt.eclipseprojects.io"
//#define MQTT_APP_URI "mqtt://iot.eclipse.org"


/**
 * Messages IDs for the MQTT application task
 * @note Can be expanded for application requirements
 */
typedef enum{
	MQTT_APP_MSG_SEND_DATA = 0,
} mqtt_app_msg_e;

/**
 * Structure for the message queue
 * @note Can be expanded for application requirements
 */
typedef struct{
	mqtt_app_msg_e	msgID;
} mqtt_app_queue_msg_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the mqtt_app_msg_e enum
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE
 */
BaseType_t mqtt_app_send_message(mqtt_app_msg_e msgID);

/**
 * Starts the MQTT application
 */
void mqtt_app_start(void);


#endif /* MAIN_MQTT_APP_H_ */
