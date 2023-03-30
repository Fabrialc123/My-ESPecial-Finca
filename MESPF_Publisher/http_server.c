/*
 * http_server.c
 *
 *  Created on: 14 jun. 2022
 *      Author: fabri
 */


#include "http_server.h"
#include "task_common.h"
#include "wifi_app.h"
#include "recollecter.h"
#include "gpios_manager.h"
#include "sensors_manager.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "sys/param.h"

#include <log.h>
#include <wifi_app.h>
#include <status.h>
#include <mqtt/mqtt_app.h>


static const char TAG[] = "http_server"; // It is used for serial console messages

static httpd_handle_t http_server_handle = NULL;

static TaskHandle_t task_http_server_monitor = NULL;

//Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

//Firmware Update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

//EMBEDDED FILES
extern const uint8_t jquery_3_3_1_min_js_start[]	asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]		asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]				asm("_binary_index_html_start");
extern const uint8_t index_html_end[]				asm("_binary_index_html_end");
extern const uint8_t app_css_start[]				asm("_binary_app_css_start");
extern const uint8_t app_css_end[]					asm("_binary_app_css_end");
extern const uint8_t app_js_start[]					asm("_binary_app_js_start");
extern const uint8_t app_js_end[]					asm("_binary_app_js_end");

/**
 * ESP32 Timer configuration passed to esp_timer_create
 */
const esp_timer_create_args_t fw_update_reset_args = {
		.callback = &http_server_fw_update_reset_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

/**
 * Checks g_fw_update_status and creates fw_update_reset Timer
 */
static void http_server_fw_update_reset_timer(void){
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL){
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update succesful, starting fw_update_reset_timer");

		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));

	}else{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update FAILED!");
	}
}

/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task
 */
static void http_server_monitor(void *pvParameters){
	http_server_queue_msg_t msg;

	for(;;){
		if(xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY)){
			switch(msg.msgID){
				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG,"HTTP_MSG_WIFI_CONNECT_INIT");
					break;
				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG,"HTTP_MSG_WIFI_CONNECT_SUCCESS");
					break;
				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG,"HTTP_MSG_WIFI_CONNECT_FAIL");
					break;

				case HTTP_MSG_OTA_UPDATE_SUCCESS:
					ESP_LOGI(TAG,"HTTP_MSG_OTA_UPDATE_SUCCESS");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();

					break;
				case HTTP_MSG_OTA_UPDATE_FAIL:
					ESP_LOGI(TAG,"HTTP_MSG_OTA_UPDATE_FAIL");
					g_fw_update_status = OTA_UPDATE_FAIL;
					break;

				default:
					break;
			}
		}
	}
}


/**
 * EMBEDDED FILES HANDLERS
 */


static esp_err_t http_server_jquery_handler(httpd_req_t *req){
	//ESP_LOGI(TAG, "JQuery Requested");

	httpd_resp_set_type(req, "application/javascript"); // To see other types go above to the function definition in esp_http_server.h
	httpd_resp_send(req, (const char *) jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

static esp_err_t http_server_index_html_handler(httpd_req_t *req){
	//ESP_LOGI(TAG, "index.html Requested");

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT); // To see other types go above to the function definition in esp_http_server.h
	httpd_resp_send(req, (const char *) index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

static esp_err_t http_server_app_css_handler(httpd_req_t *req){
	//ESP_LOGI(TAG, "app.css Requested");

	httpd_resp_set_type(req, "text/css"); // To see other types go above to the function definition in esp_http_server.h
	httpd_resp_send(req, (const char *) app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

static esp_err_t http_server_app_js_handler(httpd_req_t *req){
	//ESP_LOGI(TAG, "app.js Requested");

	httpd_resp_set_type(req, "application/javascript"); // To see other types go above to the function definition in esp_http_server.h
	httpd_resp_send(req, (const char *) app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}


/**
 * OTA files handlers
 */

static esp_err_t http_server_OTA_update_handler(httpd_req_t *req){
	esp_ota_handle_t ota_handle;
	char ota_buff[1024];
	int content_len = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	ESP_LOGI(TAG, "OTA_update Requested");

	do{
		if((recv_len = httpd_req_recv(req, ota_buff, MIN(content_len, sizeof(ota_buff)))) < 0){
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT){
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
				continue; // Retry receiving if timeout occurred
			}
			ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other Error %d", recv_len);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d \r" , content_received, content_len);

		if (!is_req_body_started){ // Is this the first data we receive?
			is_req_body_started = true;

			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;	// Get the location of the .bin file content (remove the web from the data)
			int body_part_len = recv_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d \r", content_len);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK){
				printf("http_server_OTA_update_handler: Error with OTA begin, canceling OTA \r\n");
				return ESP_FAIL;
			}
			else{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%x \r\n", update_partition->subtype, update_partition->address );
			}

			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;


		}else{	// Write remaining data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			content_received += recv_len;

		}

	}while(recv_len > 0 && content_received < content_len);

	if(esp_ota_end(ota_handle) == ESP_OK){
		if(esp_ota_set_boot_partition(update_partition) == ESP_OK){
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
			flash_successful = true;

		}else{
			ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHED ERROR!");
		}

	}else{
		ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR!");
	}

	if(flash_successful) {
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESS);
	}else http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAIL);


	return ESP_OK;
}

static esp_err_t http_server_OTA_status_handler(httpd_req_t *req){
	char otaJSON[100];

	//ESP_LOGI(TAG, "OTAstatus requested");

	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, otaJSON, strlen(otaJSON));


	return ESP_OK;
}


static esp_err_t log_get_handler(httpd_req_t *req)
{
	char log[LOG_BUF_LEN];
	log_get(log);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, log, strlen(log));
    return ESP_OK;
}

static esp_err_t ConnectionsConfiguration_handler(httpd_req_t *req){
	char ConnConf[430];
	char wifi_ssid[32],wifi_pass[64];
	short int wifi_status, ntp_status, mqtt_status;
	char ntp_server[32];
	char mqtt_ip[32],mqtt_user[32],mqtt_pass[64];
	unsigned int ntp_sync;
	int mqtt_port;

	//ESP_LOGI(TAG, "ConnectionsConfiguration requested");

	wifi_app_get_conf(wifi_ssid, wifi_pass, &wifi_status);
	status_ntp_get_conf(ntp_server, &ntp_sync, &ntp_status);
	mqtt_app_get_conf(mqtt_ip, &mqtt_port,mqtt_user, mqtt_pass, &mqtt_status);

	sprintf(ConnConf, "{\"WIFI_SSID\":\"%s\",\"WIFI_PASS\":\"%s\",\"WIFI_STATUS\":%d,\"NTP_SERVER\":\"%s\",\"NTP_STATUS\":%d,\"NTP_SYNC\":%d,\"MQTT_IP\":\"%s\",\"MQTT_PORT\":%d,\"MQTT_USER\":\"%s\",\"MQTT_PASS\":\"%s\",\"MQTT_STATUS\":%d}", wifi_ssid, wifi_pass, wifi_status, ntp_server, ntp_status, ntp_sync,mqtt_ip, mqtt_port, mqtt_user, mqtt_pass, mqtt_status);

	//ESP_LOGE("TESTING","%s",ConnConf);
	
	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, ConnConf, strlen(ConnConf));


	return ESP_OK;
}

static esp_err_t setNTPConfiguration_handler(httpd_req_t *req){
	char buf[64], *server, response[30];
	int recv_len, sync_time;
	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		server = strtok(buf,"\n");
		sync_time = atoi(strtok(NULL,"\n"));
		status_ntp_set_conf(server,sync_time);

		sprintf(response,"Restarted NTP Service");
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t setMQTTConfiguration_handler(httpd_req_t *req){
	char buf[140], *ip, *username, *pass, response[30];
	int recv_len, port;
	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		ip = strtok(buf,"\n");
		port = atoi(strtok(NULL,"\n"));
		username = strtok(NULL,"\n");
		pass = strtok(NULL,"\n");

		mqtt_app_set_conf(ip,port,username,pass);
		ESP_LOGE(TAG,"setMQTTConfiguration_handler: mqtt_app_set_conf OK!");
		sprintf(response,"Restarted MQTT Service");
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorsConfiguration_handler(httpd_req_t *req){

	ESP_LOGI(TAG, "SensorsConfiguration requested");

	char sensorsJSON[4096];
	memset(&sensorsJSON, 0, 4096);

	get_sensors_json(sensorsJSON);

	//ESP_LOGI(TAG, "JSON: %s", sensorsJSON);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, sensorsJSON, strlen(sensorsJSON));

	return ESP_OK;
}

static esp_err_t SensorsValues_handler(httpd_req_t *req){

	ESP_LOGI(TAG, "SensorsValues requested");

	char sensorsJSON[4096];
	memset(&sensorsJSON, 0, 4096);

	get_sensors_values_json(sensorsJSON);

	//ESP_LOGI(TAG, "JSON: %s", sensorsJSON);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, sensorsJSON, strlen(sensorsJSON));

	return ESP_OK;
}

static esp_err_t SensorsGpios_handler(httpd_req_t *req){

	ESP_LOGI(TAG, "SensorsGpios requested");

	char sensorsJSON[4096];
	memset(&sensorsJSON, 0, 4096);

	get_sensors_gpios_json(sensorsJSON);

	//ESP_LOGI(TAG, "JSON: %s", sensorsJSON);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, sensorsJSON, strlen(sensorsJSON));

	return ESP_OK;
}

static esp_err_t SensorsParameters_handler(httpd_req_t *req){

	ESP_LOGI(TAG, "SensorsParameters requested");

	char sensorsJSON[4096];
	memset(&sensorsJSON, 0, 4096);

	get_sensors_parameters_json(sensorsJSON);

	//ESP_LOGI(TAG, "JSON: %s", sensorsJSON);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, sensorsJSON, strlen(sensorsJSON));

	return ESP_OK;
}

static esp_err_t SensorsAlerts_handler(httpd_req_t *req){

	ESP_LOGI(TAG, "SensorsAlerts requested");

	char sensorsJSON[4096];
	memset(&sensorsJSON, 0, 4096);

	get_sensors_alerts_json(sensorsJSON);

	//ESP_LOGI(TAG, "JSON: %s", sensorsJSON);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, sensorsJSON, strlen(sensorsJSON));

	return ESP_OK;
}

static esp_err_t GetGpios_handler(httpd_req_t *req){

	ESP_LOGI(TAG, "GetGpios requested");

	char gpios[420];
	memset(&gpios, 0, 420);

	gpios_manager_json(gpios);

	//ESP_LOGI(TAG, "JSON: %s", sensorsJSON);

	httpd_resp_set_type(req, HTTPD_TYPE_JSON);
	httpd_resp_send(req, gpios, strlen(gpios));

	return ESP_OK;
}

static esp_err_t SensorAddition_handler(httpd_req_t *req){
	char buf[50], response[30];
	int recv_len, type, res;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		type = atoi(strtok(buf,"\n"));

		res = sensors_manager_init_sensor(type);

		if(res == 1)
			sprintf(response,"Sensor added");
		else
			sprintf(response,"Error, Sensor not added");
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorUnitAddition_handler(httpd_req_t *req){
	char buf[500], response[30], *parameter_type;
	int recv_len, i, n_gpios, *gpios, n_parameters, res;
	union sensor_value_u *parameters;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		n_gpios = atoi(strtok(buf,"\n"));

		gpios = (int*) malloc(sizeof(int) * n_gpios);

		for(int i = 0; i < n_gpios; i++){
			gpios[i] = atoi(strtok(NULL,"\n"));
		}

		n_parameters = atoi(strtok(NULL,"\n"));

		parameters = (union sensor_value_u*) malloc(sizeof(union sensor_value_u) * n_parameters);

		for(int i = 0; i < n_parameters; i++){
			parameter_type = strtok(NULL,"\n");

			if(strcmp(parameter_type, "INTEGER") == 0)
				parameters[i].ival = atoi(strtok(NULL,"\n"));
			else if(strcmp(parameter_type, "FLOAT") == 0)
				parameters[i].fval = atof(strtok(NULL,"\n"));
			else
				strcpy(parameters[i].cval, strtok(NULL,"\n"));
		}

		i =  atoi(strtok(NULL,"\n"));

		res = sensors_manager_add_sensor_unit(i, gpios, parameters);

		if(res == 1)
			sprintf(response,"Sensor unit added");
		else
			sprintf(response,"Error, Sensor unit not added");

		free(gpios);
		free(parameters);
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorDeletion_handler(httpd_req_t *req){

	char buf[50], response[30];
	int recv_len, i, res;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		i = atoi(strtok(buf,"\n"));

		res = sensors_manager_destroy_sensor(i);

		if(res == 1)
			sprintf(response,"Sensor deleted");
		else
			sprintf(response,"Error, sensor not deleted");
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorUnitDeletion_handler(httpd_req_t *req){
	char buf[50], response[30];
	int recv_len, i, j, res;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		i = atoi(strtok(buf,"\n"));
		j = atoi(strtok(NULL,"\n"));

		res = sensors_manager_delete_sensor_unit(i,j);

		if(res == 1)
			sprintf(response,"Sensor unit deleted");
		else
			sprintf(response,"Error, unit not deleted");
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorEditGpios_handler(httpd_req_t *req){
	char buf[500], response[30];
	int recv_len, i, j, n_gpios, *gpios, res;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		n_gpios = atoi(strtok(buf,"\n"));

		gpios = (int*) malloc(sizeof(int) * n_gpios);

		for(int i = 0; i < n_gpios; i++){
			gpios[i] = atoi(strtok(NULL,"\n"));
		}

		i =  atoi(strtok(NULL,"\n"));

		j =  atoi(strtok(NULL,"\n"));

		res = sensors_manager_set_gpios(i, j, gpios);

		if(res == 1)
			sprintf(response,"GPIOS changed");
		else
			sprintf(response,"Error, GPIOS not changed");

		free(gpios);
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorEditParameters_handler(httpd_req_t *req){
	char buf[500], response[30], *parameter_type;
	int recv_len, i, j, n_parameters, res;
	union sensor_value_u *parameters;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		n_parameters = atoi(strtok(buf,"\n"));

		parameters = (union sensor_value_u*) malloc(sizeof(union sensor_value_u) * n_parameters);

		for(int i = 0; i < n_parameters; i++){
			parameter_type = strtok(NULL,"\n");

			if(strcmp(parameter_type, "INTEGER") == 0)
				parameters[i].ival = atoi(strtok(NULL,"\n"));
			else if(strcmp(parameter_type, "FLOAT") == 0)
				parameters[i].fval = atof(strtok(NULL,"\n"));
			else
				strcpy(parameters[i].cval, strtok(NULL,"\n"));
		}

		i =  atoi(strtok(NULL,"\n"));

		j =  atoi(strtok(NULL,"\n"));

		res = sensors_manager_set_parameters(i, j, parameters);

		if(res == 1)
			sprintf(response,"Parameters changed");
		else
			sprintf(response,"Error, Parameters not changed");

		free(parameters);
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

static esp_err_t SensorEditAlerts_handler(httpd_req_t *req){
	char buf[500], response[30], *Selected_alert, *threshold_type;
	int recv_len, i, value, ticks, res;
	bool alert;
	union sensor_value_u upperThreshold, lowerThreshold;

	recv_len = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));

	if(recv_len >= sizeof(buf)) sprintf(response,"Too long parameters!");
	else {
		i =  atoi(strtok(buf,"\n"));

		value = atoi(strtok(NULL,"\n"));

		Selected_alert = strtok(NULL,"\n");

		if(strcmp(Selected_alert, "True") == 0)
			alert = true;
		else
			alert = false;

		ticks = atoi(strtok(NULL,"\n"));

		threshold_type = strtok(NULL,"\n");

		if(strcmp(threshold_type, "INTEGER") == 0){
			upperThreshold.ival = atoi(strtok(NULL,"\n"));
			lowerThreshold.ival = atoi(strtok(NULL,"\n"));
		}
		else if(strcmp(threshold_type, "FLOAT") == 0){
			upperThreshold.fval = atof(strtok(NULL,"\n"));
			lowerThreshold.fval = atof(strtok(NULL,"\n"));
		}
		else{
			strcpy(upperThreshold.cval, strtok(NULL,"\n"));
			strcpy(lowerThreshold.cval, strtok(NULL,"\n"));
		}

		res = sensors_manager_set_alert_values(i, value, alert, ticks, upperThreshold, lowerThreshold);

		if(res == 1)
			sprintf(response,"Alert changed");
		else
			sprintf(response,"Error, Alert not changed");
	}

	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	httpd_resp_send(req, response, strlen(response));

	return ESP_OK;
}

/**
 * Sets up default httpd server configuration
 * @return http server instance handle if successful, NULL otherwise
 */
static httpd_handle_t http_server_configure(void){
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);

	http_server_monitor_queue_handle = xQueueCreate(3,sizeof(http_server_queue_msg_t));

	config.core_id = HTTP_SERVER_TASK_CORE_ID;
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
	config. max_uri_handlers = 25;
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(TAG, "http_server_configure: Starting server on port %d with task priority %d",
			config.server_port,
			config.task_priority);

	if (httpd_start(&http_server_handle,&config) == ESP_OK){
		ESP_LOGI(TAG, "http_server_configure: Registering URI handlers");

		httpd_uri_t jquery_js = {
				.uri = "/jquery-3.3.1.min.js",
				.method = HTTP_GET ,
				.handler = http_server_jquery_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		httpd_uri_t index_html = {
				.uri = "/",
				.method = HTTP_GET ,
				.handler = http_server_index_html_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		httpd_uri_t app_css = {
				.uri = "/app.css",
				.method = HTTP_GET ,
				.handler = http_server_app_css_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		httpd_uri_t app_js = {
				.uri = "/app.js",
				.method = HTTP_GET ,
				.handler = http_server_app_js_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_js);


		httpd_uri_t OTA_update = {
				.uri = "/OTAupdate",
				.method = HTTP_POST ,
				.handler = http_server_OTA_update_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		httpd_uri_t OTA_status = {
				.uri = "/OTAstatus",
				.method = HTTP_POST ,
				.handler = http_server_OTA_status_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

	    httpd_uri_t log = {
	        .uri = "/log",
	        .method = HTTP_GET,
	        .handler = log_get_handler,
	        .user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &log));

	    httpd_uri_t ConnectionsConfiguration = {
	        .uri = "/ConnectionsConfiguration",
	        .method = HTTP_POST,
	        .handler = ConnectionsConfiguration_handler,
	        .user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &ConnectionsConfiguration));
		
		httpd_uri_t setNTPConfiguration = {
	        .uri = "/setNTPConfiguration",
	        .method = HTTP_POST,
	        .handler = setNTPConfiguration_handler,
	        .user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &setNTPConfiguration));
		
		httpd_uri_t setMQTTConfiguration = {
	        .uri = "/setMQTTConfiguration",
	        .method = HTTP_POST,
	        .handler = setMQTTConfiguration_handler,
	        .user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &setMQTTConfiguration));

	    httpd_uri_t SensorsConfiguration = {
			.uri = "/SensorsConfiguration",
			.method = HTTP_GET,
			.handler = SensorsConfiguration_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorsConfiguration));

	    httpd_uri_t SensorsValues = {
			.uri = "/SensorsValues",
			.method = HTTP_GET,
			.handler = SensorsValues_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorsValues));

	    httpd_uri_t SensorsGpios = {
			.uri = "/SensorsGpios",
			.method = HTTP_GET,
			.handler = SensorsGpios_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorsGpios));

	    httpd_uri_t SensorsParameters = {
			.uri = "/SensorsParameters",
			.method = HTTP_GET,
			.handler = SensorsParameters_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorsParameters));

	    httpd_uri_t SensorsAlerts = {
			.uri = "/SensorsAlerts",
			.method = HTTP_GET,
			.handler = SensorsAlerts_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorsAlerts));

	    httpd_uri_t GetGpios = {
			.uri = "/GetGpios",
			.method = HTTP_GET,
			.handler = GetGpios_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &GetGpios));

	    httpd_uri_t SensorAddition = {
			.uri = "/SensorAddition",
			.method = HTTP_POST,
			.handler = SensorAddition_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorAddition));

	    httpd_uri_t SensorUnitAddition = {
			.uri = "/SensorUnitAddition",
			.method = HTTP_POST,
			.handler = SensorUnitAddition_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorUnitAddition));

	    httpd_uri_t SensorDeletion = {
			.uri = "/SensorDeletion",
			.method = HTTP_POST,
			.handler = SensorDeletion_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorDeletion));

	    httpd_uri_t SensorUnitDeletion = {
			.uri = "/SensorUnitDeletion",
			.method = HTTP_POST,
			.handler = SensorUnitDeletion_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorUnitDeletion));

	    httpd_uri_t SensorEditGpios = {
			.uri = "/SensorEditGpios",
			.method = HTTP_POST,
			.handler = SensorEditGpios_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorEditGpios));

	    httpd_uri_t SensorEditParameters = {
			.uri = "/SensorEditParameters",
			.method = HTTP_POST,
			.handler = SensorEditParameters_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorEditParameters));

	    httpd_uri_t SensorEditAlerts = {
			.uri = "/SensorEditAlerts",
			.method = HTTP_POST,
			.handler = SensorEditAlerts_handler,
			.user_ctx = NULL
	    };
	    ESP_ERROR_CHECK(httpd_register_uri_handler(http_server_handle, &SensorEditAlerts));

		return http_server_handle;
	}
	return NULL;
}


void http_server_start(void){
	if (http_server_handle == NULL){
		http_server_handle = http_server_configure();
	}
}


void http_server_stop(void){
	if (http_server_handle != NULL){
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server");
		http_server_handle = NULL;
	}
	if (task_http_server_monitor != NULL){
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}


BaseType_t http_server_monitor_send_message(http_server_msg_e msgID){
	http_server_queue_msg_t msg;

	msg.msgID = msgID;

	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *arg){
	ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}
