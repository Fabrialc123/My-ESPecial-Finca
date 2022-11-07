/*
 * mqtt_commands.h
 *
 *  Created on: 6 nov. 2022
 *      Author: fabri
 */

#ifndef MAIN_MQTT_MQTT_COMMANDS_H_
#define MAIN_MQTT_MQTT_COMMANDS_H_

#include "mqtt_app.h"

void mqtt_app_process_command(char* topic,char* data);

void mqtt_app_send_info(char* topic);

void mqtt_app_send_alert(char* sensor_name, int id, char* dt);

void concatenate_topic(char* seg1, char* seg2,char* seg3,char* seg4,char* seg5,char* seg6, char* seg7,char* res);



#endif /* MAIN_MQTT_MQTT_COMMANDS_H_ */
