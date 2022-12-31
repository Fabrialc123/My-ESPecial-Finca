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
#include "status.h"


static const char TAG2[] = "MQTT_COMMANDS";

void mqtt_app_scan_command(char* src);
void mqtt_app_process_set_command (char *data);
void mqtt_app_send_resp(char *src, char *id, int resp);

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
		ESP_LOGI(TAG2,"Received a SET request (%s)",data);
		mqtt_app_process_set_command (data);
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

	mqtt_app_getID(personal_name);

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
	char data[MQTT_APP_MAX_DATA_LENGTH];
	memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);

	mqtt_app_getID(personal_name);

	strcpy(data,"\"");
	strcat(data,personal_name);
	strcat(data,"\"");

	concatenate_topic(USERS_TOPIC, src, SCAN_RESP_TOPIC, NULL, NULL, NULL, NULL, topic);


	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, data);
}

void mqtt_app_send_alert(char* sensor_name, int id, char* dt){
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char personal_name[MQTT_APP_MAX_DATA_LENGTH];
	char aux[sizeof(int)];

	memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	memset(&data,0,MQTT_APP_MAX_DATA_LENGTH);

	mqtt_app_getID(personal_name);
	concatenate_topic(SENSORS_TOPIC, personal_name, sensor_name, "1", ALERT_TOPIC, NULL, NULL, topic);

	strcpy(data, "{\"ID\":");
	sprintf(aux, "%d",id);
	strcat(data, aux);
	strcat(data, ",\"DESC\":\"");
	strcat(data,dt);
	strcat(data,"\"}");

	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, data);
}

void mqtt_app_process_set_command (char *data){
	char src[SET_COMMAND_SRC], id[SET_COMMAND_ID];
	char date[SET_1_COMMAND_DATE], time[SET_1_COMMAND_TIME];
	int num_args, count, resp;
	int sig_arg[4];
	const char delimiter[2] = ";";

	char aux[MQTT_APP_MAX_DATA_LENGTH], *tmp;
	strcpy(aux,data);

	num_args = 1;
	count = 0;
	sig_arg[0] = 0;
	tmp = aux;
	while (*tmp)
	{
		if (delimiter[0] == *tmp)
		{
			if(num_args == 4) {
				ESP_LOGE(TAG2, "mqtt_app_process_set_command, Too much ARGS!");
				return;
			}
			sig_arg[num_args] = count;
			num_args++;
		}
		count++;
		tmp++;
	}
	if (num_args < 4) {
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, Too few ARGS!");
		return;
	}

	memset(src,0,SET_COMMAND_SRC);
	if (sig_arg[1] - sig_arg[0] >= SET_COMMAND_SRC){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, SRC arg too long!");
		return;
	}
	strncpy(src,aux + sig_arg[0], sig_arg[1] - sig_arg[0]);
	if (*src == '\0') {
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, SRC arg can't be NULL!");
		return;
	}

	memset(id,0,SET_COMMAND_ID);
	if (sig_arg[2] - sig_arg[1] - 1 >= SET_COMMAND_ID) {
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID arg too long!");
		return;
	}
	strncpy(id,aux + sig_arg[1] + 1, sig_arg[2] - sig_arg[1] - 1);
	if (*id == '\0') {
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID arg can't be NULL!");
		return;
	}

	memset(date,0,SET_1_COMMAND_DATE);
	if (sig_arg[3] - sig_arg[2] != SET_1_COMMAND_DATE && sig_arg[3] - sig_arg[2] != 1) {
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, DATE arg incompatible size (%d)!",sig_arg[3] - sig_arg[2] );
		mqtt_app_send_resp(src,id,-1);
		return;
	}
	strncpy(date,aux + sig_arg[2] + 1, sig_arg[3] - sig_arg[2] - 1);

	memset(time,0,SET_1_COMMAND_TIME);
	if (count - sig_arg[3] != SET_1_COMMAND_TIME && count - sig_arg[3] != 1) {
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, TIME arg incompatible size (%d)!", count - sig_arg[3]);
		mqtt_app_send_resp(src,id,-1);
		return;
	}
	strncpy(time,aux + sig_arg[3] + 1, count - sig_arg[3] - 1);

	ESP_LOGI(TAG2,"mqtt_app_process_set_command, SRC: %s - ID: %s - DATE: %s - TIME: %s",src,id,date,time);

	resp = status_setDateTime(date,time);

	mqtt_app_send_resp(src,id,resp);
}


void mqtt_app_send_resp(char *src, char *id, int resp){
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char aux[SET_COMMAND_ID];

	strcpy(data,"{\"ID\":");
	strcat(data,id);
	strcat(data,",\"RES\":");
	sprintf(aux,"%d",resp);
	strcat(data,aux);
	strcat(data,"}");

	memset(topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	concatenate_topic(USERS_TOPIC, src,RESP_TOPIC,NULL,NULL,NULL,NULL ,topic);

	ESP_LOGI(TAG2,"mqtt_app_send_resp, sending RESP: %d to SRC: %s with ID: %s",resp,src,id);
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

