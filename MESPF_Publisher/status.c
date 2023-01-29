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
#include <esp_sntp.h>


static const char TAG2[] = "STATUS";

time_t start;
double upTimeAGG = 0;

static pthread_mutex_t mutex_STATUS;

static void initialize_sntp(void)
{
    ESP_LOGI(TAG2, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "192.168.3.51");
    sntp_setservername(0, NTP_SERVERNAME);
    //sntp_setservername(0, "pool.ntp.org");
    //sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    //sntp_set_sync_interval(1*3600*1000);
    sntp_set_sync_interval(NTP_SECSTOSYNC*1000);

    sntp_init();

    ESP_LOGI(TAG2, "Waiting response from NTP Server...");
    vTaskDelay(1000);
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        vTaskDelay(500);
    }
    ESP_LOGI(TAG2, "Received response from NTP Server!");
}

void status_start(){
	struct tm timeInfo;
	char strftime_buf[64];

	if(pthread_mutex_init (&mutex_STATUS, NULL) != 0){
	 ESP_LOGE(TAG2,"Failed to initialize the status mutex");
	}

	// Set timezone to Spain Standard Time
	setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
	tzset();

	initialize_sntp();

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


int status_setDateTime(const char *date,const char *timeC){
	struct tm timeInfo, timeInfoNOW;
	struct timeval tv_now;
	time_t now;

	time(&now);
	localtime_r(&now, &timeInfoNOW);

	strptime(date,"%d/%m/%Y",&timeInfo);
	strptime(timeC,"%H:%M:%S",&timeInfo);

	if (*date == '\0'){
		timeInfo.tm_mday = timeInfoNOW.tm_mday;
		timeInfo.tm_mon = timeInfoNOW.tm_mon;
		timeInfo.tm_year = timeInfoNOW.tm_year;
	}
	else if (timeInfo.tm_mday < 0 || timeInfo.tm_mday > 31 || timeInfo.tm_mon < 0 || timeInfo.tm_mon > 11 ||timeInfo.tm_year < 0 || timeInfo.tm_year > 1100){
		ESP_LOGE(TAG2,"status_setDateTime, invalid DATE format");
		return -2;
	}

	if(*timeC == '\0'){
		timeInfo.tm_sec = timeInfoNOW.tm_sec;
		timeInfo.tm_min = timeInfoNOW.tm_min;
		timeInfo.tm_hour = timeInfoNOW.tm_hour;

	}else if (timeInfo.tm_sec < 0 || timeInfo.tm_sec > 60 || timeInfo.tm_min < 0 || timeInfo.tm_min > 59 || timeInfo.tm_hour < 0 || timeInfo.tm_hour > 23){
		ESP_LOGE(TAG2,"status_setDateTime, invalid TIME format");
		return -2;
	}

	if (!rtc_setDateTime(&timeInfo)) ESP_LOGE(TAG2,"rtc_setDateTime failed!");

	pthread_mutex_lock(&mutex_STATUS);
	upTimeAGG = difftime(now, start) + upTimeAGG;
	start = mktime(&timeInfo);
	tv_now.tv_sec = start;
	tv_now.tv_usec = 0;
	settimeofday(&tv_now, NULL);
	pthread_mutex_unlock(&mutex_STATUS);

	return 1;
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

	tm = (char*)malloc(sizeof(char)*20);
	memset(tm,0,20);

	utm = (char*)malloc(sizeof(char)*15);
	memset(utm,0,15);


	wifi_app_getIP(myIP);
	mqtt_app_getID(myID);
	status_getDateTime(tm);
	status_getUpTime(utm);


	number_of_values = 4;
	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "STATUS");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;

	aux.sensor_values[0].showOnLCD = STATUS_SHOW_IP_ON_LCD;
	aux.sensor_values[0].sensor_value_type = STRING;
	strcpy(aux.sensor_values[0].valueName,"IP");
	strcpy(aux.sensor_values[0].sensor_value.cval, myIP);

	aux.sensor_values[1].showOnLCD = STATUS_SHOW_ID_ON_LCD;
	aux.sensor_values[1].sensor_value_type = STRING;
	strcpy(aux.sensor_values[1].valueName,"ID");
	strcpy(aux.sensor_values[1].sensor_value.cval, myID);

	aux.sensor_values[2].showOnLCD = STATUS_SHOW_DATE_ON_LCD;
	aux.sensor_values[2].sensor_value_type = STRING;
	strcpy(aux.sensor_values[2].valueName,"TM");
	strcpy(aux.sensor_values[2].sensor_value.cval, tm);

	aux.sensor_values[3].showOnLCD = STATUS_SHOW_UPTIME_ON_LCD;
	aux.sensor_values[3].sensor_value_type = STRING;
	strcpy(aux.sensor_values[3].valueName,"UT");
	strcpy(aux.sensor_values[3].sensor_value.cval, utm);

	free(myIP);
	free(myID);
	free(tm);
	free(utm);


	return aux;
}
