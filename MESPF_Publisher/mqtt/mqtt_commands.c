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
#include "cjson.h"

#include "mqtt_app.h"
#include "mqtt_topics.h"
#include "recollecter.h"
#include "status.h"


static const char TAG2[] = "MQTT_COMMANDS";

void mqtt_app_scan_command(char* src);
void mqtt_app_set_command (char *sensor_name,char *sensor_unit,char *data);
void mqtt_app_send_resp(char *src, int id, int resp);

void mqtt_app_process_command(char* topic,char* data){
	int aux, tl;
	char sensor_name[CHAR_LENGTH], sensor_unit[CHAR_LENGTH];

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

		mqtt_app_set_command (sensor_name,sensor_unit,data);
	}
	else{
		ESP_LOGE(TAG2, "mqtt_app_process_command: NOT SUPPORTED COMMAND %s",topic);
	}

}

void mqtt_app_send_info(char* topic){
	int i,j, recollecters ,len, sensors;
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char personal_name[MQTT_APP_MAX_DATA_LENGTH];
	char sensorName[CHAR_LENGTH];
	char sensor_topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char num_sensor[CHAR_LENGTH];

	mqtt_app_getID(personal_name);

	recollecters = get_recollecters_size();
	for (i = 0; i < recollecters;i++){
		memset(&sensor_topic, 0, MQTT_APP_MAX_TOPIC_LENGTH);
		free(get_sensor_data(i,&sensors));
		for(j = 0; j < sensors;j++){
			memset(&data, 0, MQTT_APP_MAX_DATA_LENGTH);
			memset(&sensorName, 0, CHAR_LENGTH);
			memset(&num_sensor,0,CHAR_LENGTH);
			len = get_sensor_data_cjson (i,j, data, sensorName);
			if (len < 0 || len >= MQTT_APP_MAX_DATA_LENGTH){
				ESP_LOGE(TAG2, "ERROR in get_sensor_data(sensor_id = %d)", i);
				return;
			}

			itoa(j+1,num_sensor,10);

			if (strcmp(topic,SENSORS_TOPIC) == 0){
				concatenate_topic(SENSORS_TOPIC, personal_name, sensorName, num_sensor, INFO_TOPIC, NULL, NULL, sensor_topic);
			}else {
				concatenate_topic(USERS_TOPIC,topic,REFRESH_RESP_TOPIC, personal_name, sensorName, num_sensor, INFO_TOPIC, sensor_topic);
			}
			mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, sensor_topic, data);
		}
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

void mqtt_app_set_command (char *sensor_name,char *sensor_unit,char *data){
	struct cJSON *jobj, *aux;
	char src[SET_COMMAND_SRC], arg1[CHAR_LENGTH], arg2[CHAR_LENGTH],arg3[CHAR_LENGTH],arg4[CHAR_LENGTH];
	int id, opt, resp, sensor_id;



	jobj = cJSON_Parse(data);

	resp = -1;

	if (!cJSON_HasObjectItem(jobj,"USER")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, USER not defined!");
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"USER");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= SET_COMMAND_SRC)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, USER is not STRING or too long!");
		return;
	}
	strcpy(src, cJSON_GetStringValue(aux));


	if (!cJSON_HasObjectItem(jobj,"ID")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID not defined!");
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ID");
	if(!cJSON_IsNumber(aux)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID is not NUMBER!");
		return;
	}
	id = (int)cJSON_GetNumberValue(aux);



	if (!cJSON_HasObjectItem(jobj,"OPT")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, OPT not defined!");
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"OPT");
	if(!cJSON_IsNumber(aux)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, OPT is not NUMBER!");
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	opt = (int)cJSON_GetNumberValue(aux);

	if (!cJSON_HasObjectItem(jobj,"ARG1")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG1 not defined!");
		mqtt_app_send_resp(src,id,resp);
	}
	aux = cJSON_GetObjectItem(jobj,"ARG1");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG1 is not STRING or too long!");
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg1,cJSON_GetStringValue(aux));


	if (!cJSON_HasObjectItem(jobj,"ARG2")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG2 not defined!");
		mqtt_app_send_resp(src,id,resp);
	}
	aux = cJSON_GetObjectItem(jobj,"ARG2");
	if(!cJSON_IsString(aux)|| (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG2 is not STRING or too long!");
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg2,cJSON_GetStringValue(aux));

	if (!cJSON_HasObjectItem(jobj,"ARG3")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG3 not defined!");
		mqtt_app_send_resp(src,id,resp);
	}
	aux = cJSON_GetObjectItem(jobj,"ARG3");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG3 is not STRING or too long!");
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg3,cJSON_GetStringValue(aux));

	if (!cJSON_HasObjectItem(jobj,"ARG4")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG4 not defined!");
		mqtt_app_send_resp(src,id,resp);
	}
	aux = cJSON_GetObjectItem(jobj,"ARG4");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG4 is not STRING or too long!");
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg4,cJSON_GetStringValue(aux));

	/*
	ESP_LOGE(TAG2,"mqtt_app_set_command, SRC: %s", src);
	ESP_LOGE(TAG2,"mqtt_app_set_command, ID: %d", id);
	ESP_LOGE(TAG2,"mqtt_app_set_command, OPT: %d", opt);
	ESP_LOGE(TAG2,"mqtt_app_set_command, ARG1: %s", arg1);
	ESP_LOGE(TAG2,"mqtt_app_set_command, ARG2: %s", arg2);
	ESP_LOGE(TAG2,"mqtt_app_set_command, ARG3: %s", arg3);
	ESP_LOGE(TAG2,"mqtt_app_set_command, ARG4: %s", arg4);
	*/


	sensor_id = sensors_manager_get_sensor_id_by_name(sensor_name);


	/*
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
	*/

	cJSON_Delete(jobj);
	mqtt_app_send_resp(src,id,resp);
}


void mqtt_app_send_resp(char *src, int id, int resp){
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	//char aux[SET_COMMAND_ID];
	char *aux;
	struct cJSON *jobj;

	jobj = cJSON_CreateObject();

	cJSON_AddNumberToObject(jobj,"ID",id);
	cJSON_AddNumberToObject(jobj,"RES",resp);

	aux = cJSON_Print(jobj);

	/*
	strcpy(data,"{\"ID\":");
	strcat(data,id);
	strcat(data,",\"RES\":");
	sprintf(aux,"%d",resp);
	strcat(data,aux);
	strcat(data,"}");
	*/

	memset(topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	concatenate_topic(USERS_TOPIC, src,RESP_TOPIC,NULL,NULL,NULL,NULL ,topic);

	ESP_LOGI(TAG2,"mqtt_app_send_resp, sending RESP: %d to SRC: %s with ID: %d",resp,src,id);
	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, data);

	//ESP_LOGE(TAG2,"mqtt_app_send_resp, JSON: %s", aux);

	free(aux);
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

