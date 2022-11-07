/*
 * mqtt_commands.c
 *
 *  Created on: 25 oct. 2022
 *      Author: fabri
 */
#include "mqtt_commands.h"
#include "string.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "mqtt_app.h"
#include "mqtt_topics.h"
#include "recollecter.h"


static const char TAG2[] = "MQTT_COMMANDS";

void mqtt_app_scan_command(char* src);

void mqtt_app_process_command(char* topic,char* data){
	int aux, tl;

	tl = strlen(topic);

	ESP_LOGI(TAG2, "mqtt_app_process_command");

	if(tl >= (aux=strlen(SCAN_TOPIC)) && strncmp(&topic[tl - aux],SCAN_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a SCAN request from topic %s to topic %s",topic,data);
		mqtt_app_scan_command(data);

	}else if(tl >= (aux=strlen(REFRESH_TOPIC)) && strncmp(&topic[tl - aux],REFRESH_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a REFRESH request from topic %s to topic %s",topic,data);
		mqtt_app_send_info(data);

	}else if(tl >= (aux=strlen(SET_TOPIC)) && strncmp(&topic[tl - aux],SET_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a SET request from topic %s to topic %s",topic,data);
	}
	else{
		ESP_LOGE(TAG2, "mqtt_app_process_command: NOT SUPPORTED COMMAND %s",topic);
	}

}

void mqtt_app_send_info(char* topic){
	int i, recollecters ,len;
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char personal_name[MQTT_APP_MAX_DATA_LENGTH];
	char sensorName[CHAR_LENGTH];
	char sensor_topic[MQTT_APP_MAX_TOPIC_LENGTH];

	mqtt_app_get_personal_name(personal_name);

	recollecters = get_recollecters_size();
	for (i = 0; i < recollecters;i++){
		memset(&data, 0, MQTT_APP_MAX_DATA_LENGTH);
		memset(&sensorName, 0, CHAR_LENGTH);
		memset(&sensor_topic, 0, MQTT_APP_MAX_TOPIC_LENGTH);

		len = get_sensor_data_json (i, &data);
		if (len < 0 || len >= MQTT_APP_MAX_DATA_LENGTH){
			ESP_LOGE(TAG2, "ERROR in get_sensor_data(sensor_id = %d)", i);
			return;
		}
		get_sensor_data_name(i,sensorName);
		if (strcmp(topic,SENSORS_TOPIC) == 0){
			concatenate_topic(SENSORS_TOPIC, personal_name, sensorName, "1", INFO_TOPIC, NULL, NULL, sensor_topic);
		}else {
			concatenate_topic(USERS_TOPIC,topic,REFRESH_RESP_TOPIC, personal_name, sensorName, "1", INFO_TOPIC, sensor_topic);
		}
		mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, sensor_topic, data);
	}
}

void mqtt_app_scan_command(char* src){
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char personal_name[MQTT_APP_MAX_DATA_LENGTH];
	memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	/*strcpy(aux,topic);
	strcat(aux,"/");
	strcat(aux,SCAN_RESP_TOPIC);*/
	concatenate_topic(USERS_TOPIC, src, SCAN_RESP_TOPIC, NULL, NULL, NULL, NULL, topic);
	//esp_mqtt_client_publish(client, topic, MQTT_APP_PERSONAL_NAME, 0, MQTT_APP_QOS, 0);
	mqtt_app_get_personal_name(personal_name);
	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, personal_name);
}

void mqtt_app_send_alert(char* sensor_name, int id, char* dt){
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char personal_name[MQTT_APP_MAX_DATA_LENGTH];
	char aux[sizeof(int)];

	memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	memset(&data,0,MQTT_APP_MAX_DATA_LENGTH);

	mqtt_app_get_personal_name(personal_name);
	concatenate_topic(SENSORS_TOPIC, personal_name, sensor_name, "1", ALERT_TOPIC, NULL, NULL, topic);

	strcpy(data, "{\"ID\":");
	sprintf(aux, "%d",id);
	strcat(data, aux);
	strcat(data, ",\"DESC\":\"");
	strcat(data,dt);
	strcat(data,"\"}");

	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, data);
}



void concatenate_topic(char* seg1, char* seg2,char* seg3,char* seg4,char* seg5,char* seg6, char* seg7,char* res){

	strcat(res,seg1);

	strcat(res,"/");
	strcat(res,seg2);

	if (seg3){
		strcat(res,"/");
		strcat(res,seg3);
	}
	if (seg4){
		strcat(res,"/");
		strcat(res,seg4);
	}
	if (seg5){
		strcat(res,"/");
		strcat(res,seg5);
	}
	if (seg6){
		strcat(res,"/");
		strcat(res,seg6);
	}
	if (seg7){
		strcat(res,"/");
		strcat(res,seg7);
	}
}

