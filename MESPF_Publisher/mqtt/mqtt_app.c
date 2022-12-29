/* CREATED BY FABRIZIO ALCARAZ ESCOBAR */

/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <mqtt/mqtt_app.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include <pthread.h>

#include "mqtt_topics.h"
#include "task_common.h"
#include "recollecter.h"

#include "mqtt_commands.h"
#include "status.h"

static const char TAG[] = "MQTT_APP";

char *MQTT_APP_PERSONAL_NAME;

static QueueHandle_t mqtt_app_queue_handle;
static esp_mqtt_client_handle_t client;

static int sended_packs;
static int received_packs;

static pthread_mutex_t mutex_MQTT_APP_MQTT_CONNECTED;
static int MQTT_CONNECTED = 0;

static TaskHandle_t MQTT_APP_TASK_HANDLE_TASK;
static TaskHandle_t MQTT_APP_TASK_HANDLE_DATA_SENDER;


// FUNCTION COPIED FROM platform_create_id_string() (platform_esp32_idf.c)
static void set_personal_topic_name(void){
	//char aux[32];
    uint8_t mac[6];
    MQTT_APP_PERSONAL_NAME = calloc(1, MQTT_APP_MAX_TOPIC_LENGTH);
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(MQTT_APP_PERSONAL_NAME, "ESP32_%02x%02X%02X", mac[3], mac[4], mac[5]);

}

void mqtt_app_format_data(char *src){
	char timestamp[20];
	char aux[MQTT_APP_MAX_DATA_LENGTH];
	status_getDateTime(timestamp);
	strcpy(aux, "{\"DT\":");
	strcat(aux,src);
	strcat(aux,",\"TS\":\"");
	strcat(aux,timestamp);
	strcat(aux,"\"}");

	strcpy(src,aux);

}

int is_mqtt_connected(){
	int aux;

	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
	aux = MQTT_CONNECTED;
	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

	return aux;
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_app_disconnect(void){
	int status;

	status = esp_mqtt_client_disconnect(client);
	ESP_LOGE(TAG,"Client disconnected (%d)", status);
	status = esp_mqtt_client_stop(client);
	ESP_LOGE(TAG,"Client stopped (%d)", status);
	status = esp_mqtt_client_destroy(client);
	ESP_LOGE(TAG,"Client destroyed (%d)", status);

	vQueueDelete(mqtt_app_queue_handle);

	free(MQTT_APP_PERSONAL_NAME);

	vTaskDelete(MQTT_APP_TASK_HANDLE_DATA_SENDER);
	vTaskDelete(MQTT_APP_TASK_HANDLE_TASK);

}

static void mqtt_app_data_sender (void *pvParameters){
	 ESP_LOGE("MQTT_APP_DATA_SENDER","STACK SIZE: %d / %d",uxTaskGetStackHighWaterMark(NULL), MQTT_APP_DATA_SENDER_STACK_SIZE);
	 ESP_LOGI("MQTT_APP_DATA_SENDER", "Sending data every %d seconds.", MQTT_APP_TIME_TO_SEND_DATA/100);
	 vTaskDelay(MQTT_APP_TIME_TO_SEND_DATA);
	 for(;;){
		 ESP_LOGI("MQTT_APP_DATA_SENDER","Sending data...");
		 mqtt_app_send_info(SENSORS_TOPIC);
		 vTaskDelay(MQTT_APP_TIME_TO_SEND_DATA);
	 }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	int msg_id;
	char topic[MQTT_APP_MAX_TOPIC_LENGTH];
	char data[MQTT_APP_MAX_DATA_LENGTH];

    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);

    	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
    	MQTT_CONNECTED = 1;
    	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);


		/*strcpy(aux,MQTT_APP_COMMANDS_TOPIC);
		strcat(aux,"/#");		// Subscribes to all subtopics*/
    	concatenate_topic(SENSORS_TOPIC, MQTT_APP_PERSONAL_NAME, "+", "+", COMMANDS_TOPIC, "+",NULL, topic);
		msg_id = esp_mqtt_client_subscribe(client, topic, MQTT_APP_QOS);
		ESP_LOGI(TAG, "sent subscribe successful to topic %s , msg_id=%d",topic, msg_id);

		msg_id = esp_mqtt_client_subscribe(client, SCAN_TOPIC, MQTT_APP_QOS);
		ESP_LOGI(TAG, "sent subscribe successful to topic %s , msg_id=%d",SCAN_TOPIC, msg_id);


		xTaskCreatePinnedToCore(&mqtt_app_data_sender, "mqtt_app_data_sender", MQTT_APP_DATA_SENDER_STACK_SIZE, NULL, MQTT_APP_DATA_SENDER_PRIORITY, &MQTT_APP_TASK_HANDLE_DATA_SENDER, MQTT_APP_DATA_SENDER_CORE_ID);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
        	MQTT_CONNECTED = -1;
        	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

			ESP_LOGE(TAG, "Retrying reconnect in %d seconds...", MQTT_APP_SECONDS_TO_RECONNECT);
			vTaskDelay(MQTT_APP_SECONDS_TO_RECONNECT);
			esp_mqtt_client_reconnect(client);

        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        memset(&topic,0,MQTT_APP_MAX_TOPIC_LENGTH);
        memset(&data,0,MQTT_APP_MAX_DATA_LENGTH);

        strncpy(topic,event->topic,event->topic_len);
        strncpy(data,event->data, event->data_len);

        mqtt_app_send_message(MQTT_APP_MSG_PROCESS_COMMAND, topic, data);

        break;

    case MQTT_EVENT_BEFORE_CONNECT:
    	ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
    	break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED){
        	ESP_LOGE(TAG,"The broker refused the connection!");
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id: %d", event->event_id);
        break;
    }
}

static void mqtt_app_task(void *pvParameters){
	mqtt_app_queue_msg_t msg;
	/*
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "broker.hivemq.com",
    };
*/
    esp_mqtt_client_config_t mqtt_cfg = {
#ifdef MQTT_APP_HOST
    	.host = MQTT_APP_HOST,
#else
        .uri = MQTT_APP_URI,
#endif
		.port = MQTT_APP_PORT,
    };

    ESP_LOGE(TAG,"STACK SIZE: %d / %d",uxTaskGetStackHighWaterMark(NULL), MQTT_APP_TASK_STACK_SIZE);

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

	for(;;){
		if (xQueueReceive(mqtt_app_queue_handle, &msg, portMAX_DELAY)){
			switch(msg.msgID){
				case MQTT_APP_MSG_PUBLISH_DATA:
					ESP_LOGI(TAG, "MQTT_APP_MSG_PUBLISH_DATA to topic %s",msg.src);

					//mqtt_app_send_data(msg.data);
					mqtt_app_format_data(msg.data);
					esp_mqtt_client_publish(client, msg.src, msg.data, 0, MQTT_APP_QOS, 0);
					ESP_LOGI(TAG, "sent publish successful");

					break;

				case MQTT_APP_MSG_SUBSCRIBE:
					ESP_LOGI(TAG, "MQTT_APP_MSG_SUBSCRIBE to topic %s", msg.data);

				    esp_mqtt_client_subscribe(client, msg.data, MQTT_APP_QOS);
				    ESP_LOGI(TAG, "sent subscribe to topic %s ",msg.data);

					break;

				case MQTT_APP_MSG_DISCONNECT:
					ESP_LOGE(TAG, "MQTT_APP_MSG_DISCONNECT");

					mqtt_app_disconnect();

					break;

				case MQTT_APP_MSG_PROCESS_COMMAND:
					ESP_LOGI(TAG, "MQTT_APP_MSG_PROCESS_COMMAND");

					mqtt_app_process_command(msg.src,msg.data);

					break;

				default:
					break;
			}
		}
	}
}

BaseType_t mqtt_app_send_message(mqtt_app_msg_e msgID, char src[MQTT_APP_MAX_TOPIC_LENGTH], char data[MQTT_APP_MAX_DATA_LENGTH]){
	mqtt_app_queue_msg_t msg;

	msg.msgID = msgID;
	if (src != NULL){
		strcpy(msg.src,src);
	}

	if (strlen(data) < MQTT_APP_MAX_DATA_LENGTH ){
		strcpy(msg.data, data);
	}else
		strcpy(msg.data,"");

	return xQueueSend(mqtt_app_queue_handle, &msg, portMAX_DELAY);
}

void mqtt_app_start(void)
{
//    esp_log_level_set("*", ESP_LOG_INFO);
//    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
//    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
//    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
//    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
//    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
//    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

	sended_packs = 0;
	received_packs = 0;

	if(pthread_mutex_init (&mutex_MQTT_APP_MQTT_CONNECTED, NULL) != 0){
	 ESP_LOGE(TAG,"Failed to initialize the MQTT_CONNECTED mutex");
	}

	set_personal_topic_name();

    mqtt_app_queue_handle = xQueueCreate(MQTT_APP_QUEUE_HANDLE_SIZE, sizeof(mqtt_app_queue_msg_t));

    xTaskCreatePinnedToCore(&mqtt_app_task, "mqtt_app_task", MQTT_APP_TASK_STACK_SIZE, NULL, MQTT_APP_TASK_PRIORITY, &MQTT_APP_TASK_HANDLE_TASK, MQTT_APP_TASK_CORE_ID);

    ESP_LOGI(TAG,"Waiting for mqtt broker connection");
    while(!is_mqtt_connected()){
    	vTaskDelay(10);
    }

}

void mqtt_app_getID(char *id){
	strcpy(id, MQTT_APP_PERSONAL_NAME);
}

