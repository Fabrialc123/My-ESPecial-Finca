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

#include <mqtt/mqtt_app.h>
#include <mqtt/mqtt_topics.h>
#include <recollecter.h>
#include <sensors_manager.h>
#include <status.h>


static const char TAG2[] = "MQTT_COMMANDS";

void mqtt_app_refresh_command (char *data);
void mqtt_app_scan_command(char* src);
void mqtt_app_set_command (char *sensor_name,int sensor_unit,char *data);
void mqtt_app_getconf_command (char *sensor_name,int sensor_unit,char *data);
void mqtt_app_send_resp(char *src, int id, int resp);

void mqtt_app_process_command(char* topic,char* data){
	int aux, tl, sensor_unit, i;
	char sensor_name[CHAR_LENGTH];
	char delimiter[] = "/";

	tl = strlen(topic);

	ESP_LOGI(TAG2, "mqtt_app_process_command");

	if(tl >= (aux=strlen(SCAN_TOPIC)) && strncmp(&topic[tl - aux],SCAN_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a SCAN request from topic %s to topic %s",topic,data);
		mqtt_app_scan_command(data);

	}else if(tl >= (aux=strlen(REFRESH_TOPIC)) && strncmp(&topic[tl - aux],REFRESH_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a REFRESH request from topic %s to topic %s",topic,data);
		mqtt_app_refresh_command(data);

	}else if(tl >= (aux=strlen(SET_TOPIC)) && strncmp(&topic[tl - aux],SET_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a SET request (%s)",data);

		strtok(topic,delimiter);
		for(i = 0; i < 2; i++) strtok(NULL,delimiter);

	    strncpy(sensor_name, strtok(NULL,delimiter),CHAR_LENGTH);
	    sensor_unit = atoi(strtok(NULL,delimiter)) - 1;

		mqtt_app_set_command (sensor_name,sensor_unit,data);
	}else if(tl >= (aux=strlen(GETCONF_TOPIC)) && strncmp(&topic[tl - aux],GETCONF_TOPIC,aux) == 0){
		ESP_LOGI(TAG2,"Received a GETCONF request (%s)",data);

		strtok(topic,delimiter);
		for(i = 0; i < 2; i++) strtok(NULL,delimiter);

	    strncpy(sensor_name, strtok(NULL,delimiter),CHAR_LENGTH);
	    sensor_unit = atoi(strtok(NULL,delimiter)) - 1;

		mqtt_app_getconf_command (sensor_name,sensor_unit,data);
	}
	else{
		ESP_LOGE(TAG2, "mqtt_app_process_command: NOT SUPPORTED COMMAND %s",topic);
	}

}

void mqtt_app_refresh_command (char *data){
	struct cJSON *jobj, *aux;
	int id, resp;
	char src[SET_COMMAND_SRC];

	jobj = cJSON_Parse(data);

	resp = -1;

	if (!cJSON_HasObjectItem(jobj,"USER")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, USER not defined!");
		cJSON_Delete(jobj);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"USER");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= SET_COMMAND_SRC)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, USER is not STRING or too long!");
		cJSON_Delete(jobj);
		return;
	}
	strcpy(src, cJSON_GetStringValue(aux));


	if (!cJSON_HasObjectItem(jobj,"ID")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID not defined!");
		cJSON_Delete(jobj);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ID");
	if(!cJSON_IsNumber(aux)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID is not NUMBER!");
		cJSON_Delete(jobj);
		return;
	}
	id = (int)cJSON_GetNumberValue(aux);

	mqtt_app_send_info(src);
	mqtt_app_send_resp(src,id,resp);

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
		free(get_sensor_data(i,&sensors));
		for(j = 0; j < sensors;j++){
			memset(&sensor_topic, 0, MQTT_APP_MAX_TOPIC_LENGTH);
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

void mqtt_app_send_alert(char* sensor_name, int pos, int id, char* dt){
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char data[MQTT_APP_MAX_DATA_LENGTH];
	char personal_name[MQTT_APP_MAX_DATA_LENGTH];
	char aux[CHAR_LENGTH];

	memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	memset(&data,0,MQTT_APP_MAX_DATA_LENGTH);

	mqtt_app_getID(personal_name);
	sprintf(aux,"%d",pos + 1);
	concatenate_topic(SENSORS_TOPIC, personal_name, sensor_name, aux, ALERT_TOPIC, NULL, NULL, topic);

	memset(&aux, 0, CHAR_LENGTH);

	strcpy(data, "{\"ID\":");
	sprintf(aux, "%d",id);
	strcat(data, aux);
	strcat(data, ",\"DESC\":\"");
	strcat(data,dt);
	strcat(data,"\"}");

	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, data);
}

void mqtt_app_set_command (char *sensor_name,int sensor_unit,char *data){
	struct cJSON *jobj, *aux;
	char src[SET_COMMAND_SRC], arg1[CHAR_LENGTH], arg2[CHAR_LENGTH],arg3[CHAR_LENGTH],arg4[CHAR_LENGTH], auxC[CHAR_LENGTH], reason[50];
	int id, opt, resp, sensor_id, num_sensors, i;
	int value,gpios[5];
	bool enable;
	sensor_data_t *sensor_data;
	union sensor_value_u sensor_value[5];
	sensor_additional_parameters_info_t *sensor_parameters;



	jobj = cJSON_Parse(data);

	resp = -1;

	if (!cJSON_HasObjectItem(jobj,"USER")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, USER not defined!");
		cJSON_Delete(jobj);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"USER");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= SET_COMMAND_SRC)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, USER is not STRING or too long!");
		cJSON_Delete(jobj);
		return;
	}
	strcpy(src, cJSON_GetStringValue(aux));


	if (!cJSON_HasObjectItem(jobj,"ID")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID not defined!");
		cJSON_Delete(jobj);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ID");
	if(!cJSON_IsNumber(aux)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ID is not NUMBER!");
		cJSON_Delete(jobj);
		return;
	}
	id = (int)cJSON_GetNumberValue(aux);


	resp = -100;
	if (!cJSON_HasObjectItem(jobj,"OPT")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, OPT not defined!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"OPT");
	if(!cJSON_IsNumber(aux)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, OPT is not NUMBER!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	opt = (int)cJSON_GetNumberValue(aux);


	resp = -101;
	if (!cJSON_HasObjectItem(jobj,"ARG1")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG1 not defined!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ARG1");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG1 is not STRING or too long!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg1,cJSON_GetStringValue(aux));


	resp = -102;
	if (!cJSON_HasObjectItem(jobj,"ARG2")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG2 not defined!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ARG2");
	if(!cJSON_IsString(aux)|| (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG2 is not STRING or too long!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg2,cJSON_GetStringValue(aux));


	resp = -103;
	if (!cJSON_HasObjectItem(jobj,"ARG3")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG3 not defined!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ARG3");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG3 is not STRING or too long!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg3,cJSON_GetStringValue(aux));


	resp = -104;
	if (!cJSON_HasObjectItem(jobj,"ARG4")){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG4 not defined!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ARG4");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= CHAR_LENGTH)){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, ARG4 is not STRING or too long!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}
	strcpy(arg4,cJSON_GetStringValue(aux));

	resp = -106;
	sensor_id = get_sensor_id_by_name(sensor_name);
	if (sensor_id < 1){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, sensor_id not found!");
		cJSON_Delete(jobj);
		mqtt_app_send_resp(src,id,resp);
		return;
	}

	resp = -107;
	sensor_data = get_sensor_data (sensor_id, &num_sensors);
	if (sensor_data == NULL){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, SENSOR_DATA NOT FOUND (%d)", sensor_id);

		cJSON_Delete(jobj);

		mqtt_app_send_resp(src,id,resp);
		return;
	}

	resp = -108;
	ESP_LOGE(TAG2,"mqtt_app_process_set_command, num_sensors: %d",num_sensors);
	if (num_sensors == 0 || num_sensors <= sensor_unit || sensor_unit < 0){
		ESP_LOGE(TAG2, "mqtt_app_process_set_command, Invalid sensor_unit (%d) for sensor_id (%d)", sensor_unit,sensor_id);

		cJSON_Delete(jobj);
		for(i = 0; i < num_sensors ; i++) free(sensor_data[i].sensor_values);
		free(sensor_data);

		mqtt_app_send_resp(src,id,resp);
		return;
	}

	switch(opt){
		case 0:
			value = atoi(arg1);
			enable = (strcmp(arg2,"0") == 0) ? 0 : 1;

			if (value < sensor_data[0].valuesLen && value >= 0){
				if (sensor_data[0].sensor_values[value].sensor_value_type == INTEGER){
					sensor_value[0].ival = atoi(arg3);
					sensor_value[1].ival = atoi(arg4);
				}else if (sensor_data[0].sensor_values[value].sensor_value_type == FLOAT){
					sensor_value[0].fval = atof(arg3);
					sensor_value[1].fval = atof(arg4);
				}else {
					strcpy(sensor_value[0].cval, arg3);
					strcpy(sensor_value[1].cval, arg4);
				}
				ESP_LOGE(TAG2,"mqtt_app_set_command, sensor_id: %d", sensor_id);
				resp = sensors_manager_set_alert_values(sensor_id - 1,value,enable,sensor_data[0].sensor_values[value].ticks_to_alert, sensor_value[0], sensor_value[1], reason);
			}
			else {
				resp = -109;
			}
			break;

		case 1:
			gpios[0] = atoi(arg1);
			gpios[1] = atoi(arg2);
			gpios[2] = atoi(arg3);
			gpios[3] = atoi(arg4);
			resp = sensors_manager_set_gpios(sensor_id - 1, sensor_unit,gpios,reason);
			break;

		case 2:
			sensor_parameters = get_sensor_parameters(sensor_id, &num_sensors);
			if (sensor_parameters == NULL){
				ESP_LOGE(TAG2, "mqtt_app_process_set_command, SENSOR_PARAMETERS NOT FOUND (%d)", sensor_id);
				resp = -110;
			}
			else if (num_sensors == 0 || num_sensors <= sensor_unit || sensor_unit < 0){
				ESP_LOGE(TAG2, "mqtt_app_process_set_command, Invalid sensor_unit (%d) for sensor_id (%d) in GET_SENSOR_PARAMETERS", sensor_unit,sensor_id);
				resp = -111;
			}
			else{
				for(i = 0; i < sensor_parameters[0].parametersLen; i++){
					if (i == 0) strcpy(auxC,arg1);
					if (i == 1) strcpy(auxC,arg2);
					if (i == 2) strcpy(auxC,arg3);
					if (i == 3) strcpy(auxC,arg4);

					if (sensor_parameters[0].sensor_parameters[i].sensor_parameter_type == INTEGER){
						sensor_value[i].ival = atoi(auxC);
					}else if (sensor_parameters[0].sensor_parameters[i].sensor_parameter_type == FLOAT){
						sensor_value[i].ival = atof(auxC);
					}else
						strcpy(sensor_value[i].cval, auxC);
				}
				resp = sensors_manager_set_parameters(sensor_id - 1, sensor_unit, sensor_value, reason);
			}

			if (num_sensors > 0) {
				for(i = 0; i < num_sensors; i++) free(sensor_parameters[i].sensor_parameters);
				free(sensor_parameters);
			}
			break;

		default:
			resp = -404;
			break;
	}

	cJSON_Delete(jobj);
	for(i = 0; i < num_sensors ; i++) free(sensor_data[i].sensor_values);
	free(sensor_data);
	mqtt_app_send_resp(src,id,resp);
}

void mqtt_app_getconf_command(char *sensor_name,int sensor_unit,char *data){
	struct cJSON *jobj, *aux;
	char topic[MQTT_APP_MAX_TOPIC_LENGTH],src[SET_COMMAND_SRC],objName[CHAR_LENGTH],objName2[CHAR_LENGTH],objName3[CHAR_LENGTH] , *conf;
	int sensor_id,num_sensors, i, id, resp;
	sensor_gpios_info_t *gpios_info;
	sensor_additional_parameters_info_t *parameters_info;
	sensor_data_t *data_info;

	jobj = cJSON_Parse(data);

	if (!cJSON_HasObjectItem(jobj,"USER")){
		ESP_LOGE(TAG2, "mqtt_app_getconf_command, USER not defined!");
		cJSON_Delete(jobj);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"USER");
	if(!cJSON_IsString(aux) || (strlen(cJSON_GetStringValue(aux)) >= SET_COMMAND_SRC)){
		ESP_LOGE(TAG2, "mqtt_app_getconf_command, USER is not STRING or too long!");
		cJSON_Delete(jobj);
		return;
	}
	strcpy(src, cJSON_GetStringValue(aux));


	if (!cJSON_HasObjectItem(jobj,"ID")){
		ESP_LOGE(TAG2, "mqtt_app_getconf_command, ID not defined!");
		cJSON_Delete(jobj);
		return;
	}
	aux = cJSON_GetObjectItem(jobj,"ID");
	if(!cJSON_IsNumber(aux)){
		ESP_LOGE(TAG2, "mqtt_app_getconf_command, ID is not NUMBER!");
		cJSON_Delete(jobj);
		return;
	}
	id = (int)cJSON_GetNumberValue(aux);

	cJSON_Delete(jobj);


	resp = 1;

	sensor_id = get_sensor_id_by_name(sensor_name);

	if (sensor_id > 0){
		data_info = get_sensor_data (sensor_id, &num_sensors);
		if (num_sensors == 0 || sensor_unit < 0 || sensor_unit >= num_sensors){
			resp = -1;
		}
	}else resp = -2;

	jobj = cJSON_CreateObject();

	cJSON_AddNumberToObject(jobj,"ID",id);
	cJSON_AddNumberToObject(jobj,"RES",resp);

	if (resp == 1){
		for(i = 0; i < data_info[0].valuesLen; i++){
			memset(objName,0,CHAR_LENGTH);
			sprintf(objName,"UT%d",i+1);
			memset(objName2,0,CHAR_LENGTH);
			sprintf(objName2,"LT%d",i+1);
			memset(objName3,0,CHAR_LENGTH);
			sprintf(objName3,"EN%d",i+1);

			cJSON_AddBoolToObject(jobj,objName3,data_info[0].sensor_values[i].alert);
			switch (data_info[0].sensor_values[i].sensor_value_type){
				case INTEGER:
					cJSON_AddNumberToObject(jobj,objName,data_info[0].sensor_values[i].upper_threshold.ival);
					cJSON_AddNumberToObject(jobj,objName2,data_info[0].sensor_values[i].lower_threshold.ival);
					break;
				case FLOAT:
					cJSON_AddNumberToObject(jobj,objName,data_info[0].sensor_values[i].upper_threshold.fval);
					cJSON_AddNumberToObject(jobj,objName2,data_info[0].sensor_values[i].lower_threshold.fval);
					break;

				case STRING:
					cJSON_AddStringToObject(jobj,objName,data_info[0].sensor_values[i].upper_threshold.cval);
					cJSON_AddStringToObject(jobj,objName2,data_info[0].sensor_values[i].upper_threshold.cval);
					break;

				default:
					break;
			}
		}
		for(i = 0; i < num_sensors;i++) free(data_info[i].sensor_values);
		free(data_info);


		gpios_info = get_sensor_gpios(sensor_id, &num_sensors);
		if (num_sensors > sensor_unit && gpios_info != NULL){
			for(i = 0; i < gpios_info[sensor_unit].gpiosLen; i++){
				cJSON_AddNumberToObject(jobj,gpios_info[sensor_unit].sensor_gpios[i].gpioName,gpios_info[sensor_unit].sensor_gpios[i].sensor_gpio);
			}
			for(i = 0; i < num_sensors;i++) free(gpios_info[i].sensor_gpios);
			free(gpios_info);
		}


		parameters_info = get_sensor_parameters(sensor_id, &num_sensors);
		if (num_sensors > sensor_unit && parameters_info != NULL){
			for(i = 0; i < parameters_info[sensor_unit].parametersLen; i++){
				switch (parameters_info[sensor_unit].sensor_parameters[i].sensor_parameter_type){
					case INTEGER:
						cJSON_AddNumberToObject(jobj,parameters_info[sensor_unit].sensor_parameters[i].parameterName,parameters_info[sensor_unit].sensor_parameters[i].sensor_parameter.ival);
						break;
					case FLOAT:
						cJSON_AddNumberToObject(jobj,parameters_info[sensor_unit].sensor_parameters[i].parameterName,parameters_info[sensor_unit].sensor_parameters[i].sensor_parameter.fval);
						break;

					case STRING:
						cJSON_AddStringToObject(jobj,parameters_info[sensor_unit].sensor_parameters[i].parameterName,parameters_info[sensor_unit].sensor_parameters[i].sensor_parameter.cval);
						break;

					default:
						break;
				}
			}
			for(i = 0; i < num_sensors;i++) free(parameters_info[i].sensor_parameters);
			free(parameters_info);
		}
	}

	conf = cJSON_Print(jobj);

	memset(topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	concatenate_topic(USERS_TOPIC, src,RESP_TOPIC,NULL,NULL,NULL,NULL ,topic);

	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, conf);

	free(conf);
}

void mqtt_app_send_resp(char *src, int id, int resp){
	char *data; //[MQTT_APP_MAX_DATA_LENGTH];
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	//char *aux;
	struct cJSON *jobj;

	jobj = cJSON_CreateObject();

	cJSON_AddNumberToObject(jobj,"ID",id);
	cJSON_AddNumberToObject(jobj,"RES",resp);

	data = cJSON_Print(jobj);

	memset(topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
	concatenate_topic(USERS_TOPIC, src,RESP_TOPIC,NULL,NULL,NULL,NULL ,topic);

	ESP_LOGI(TAG2,"mqtt_app_send_resp, sending RESP: %d to SRC: %s with ID: %d",resp,src,id);
	mqtt_app_send_message(MQTT_APP_MSG_PUBLISH_DATA, topic, data);

	free(data);
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

