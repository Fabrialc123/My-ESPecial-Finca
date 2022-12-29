/*
 * status.c
 *
 *  Created on: 26 oct. 2022
 *      Author: fabri
 */

#include <gpios/rtc.h>
#include "status.h"
#include <pthread.h>
#include "wifi_app.h"
#include "mqtt/mqtt_app.h"
#include <time.h>
#include "esp_log.h"
#include <string.h>


static const char TAG2[] = "STATUS";

time_t start;
double upTimeAGG = 0;

static pthread_mutex_t mutex_STATUS;

void status_start(){
	struct tm timeInfo;
	struct timeval tv_now;
	char strftime_buf[64];

	if(pthread_mutex_init (&mutex_STATUS, NULL) != 0){
	 ESP_LOGE(TAG2,"Failed to initialize the status mutex");
	}

	// Set timezone to Spain Standard Time
	setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
	tzset();

	if(rtc_initialize()){
		rtc_getDateTime(&timeInfo);
		timeInfo.tm_hour += 1;
		start = mktime(&timeInfo);
		tv_now.tv_sec = start;
		tv_now.tv_usec = 0;
		settimeofday(&tv_now, NULL);
	}else {
		ESP_LOGE(TAG2, "rtc_initialize failed! Can't update time");
	}

	time(&start);
	localtime_r(&start, &timeInfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeInfo);
	ESP_LOGI(TAG2, "The current date/time in Spain is: %s", strftime_buf);

	register_recollecter(&status_recollecter);

}

bool status_getDateTime(char *dt){
	time_t now;
	struct tm timeInfo;

	time(&now);

	localtime_r(&now, &timeInfo);
	sprintf(dt, "%02d:%02d:%02d %02d/%02d/%04d",timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec
			,timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900 );
	ESP_LOGI(TAG2, "status_getDateTime: %s", dt);

	return true;
}


bool status_setDateTime(const char *dt){

	return true;
}

void status_getTime(char *tm){
	time_t now;
	struct tm timeInfo;

	time(&now);

	localtime_r(&now, &timeInfo);
	sprintf(tm, "%02d:%02d:%02d",timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
}

void status_getUpTime(char *utm){
	time_t now;
	double upTime;
	int d, h, m;

	time(&now);

	pthread_mutex_lock(&mutex_STATUS);
	upTime = difftime(now, start) + upTimeAGG;
	pthread_mutex_unlock(&mutex_STATUS);

    d =((int) upTime / 86400);
    h =(((int) upTime % 86400) / 3600);
    m =((((int) upTime % 86400) % 3600) / 60);

    sprintf(utm, "%03d %02d:%02d", d, h, m);
}


sensor_data_t status_recollecter (void){
	sensor_data_t aux;
	sensor_value_t *aux2;
	int number_of_values;

	char *myIP, *myID, *tm, *utm;

	myIP = (char*)malloc(sizeof(char)*20);
	memset(myIP,0,20);

	myID = (char*)malloc(sizeof(char)*MQTT_APP_MAX_TOPIC_LENGTH);
	memset(myID,0,MQTT_APP_MAX_TOPIC_LENGTH);

	tm = (char*)malloc(sizeof(char)*15);
	memset(tm,0,15);

	utm = (char*)malloc(sizeof(char)*15);
	memset(utm,0,15);


	wifi_app_getIP(myIP);
	mqtt_app_getID(myID);
	status_getTime(tm);
	status_getUpTime(utm);


	number_of_values = 4;
	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "STATUS");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;
	aux.sensor_values[0].sensor_value_type = STRING;
	strcpy(aux.sensor_values[0].valueName,"IP");
	strcpy(aux.sensor_values[0].sensor_value.cval, myIP);

	aux.sensor_values[1].sensor_value_type = STRING;
	strcpy(aux.sensor_values[1].valueName,"ID");
	strcpy(aux.sensor_values[1].sensor_value.cval, myID);

	aux.sensor_values[2].sensor_value_type = STRING;
	strcpy(aux.sensor_values[2].valueName,"TM");
	strcpy(aux.sensor_values[2].sensor_value.cval, tm);

	aux.sensor_values[3].sensor_value_type = STRING;
	strcpy(aux.sensor_values[3].valueName,"UT");
	strcpy(aux.sensor_values[3].sensor_value.cval, utm);

	free(myIP);
	free(myID);
	free(tm);
	free(utm);


	return aux;
}



