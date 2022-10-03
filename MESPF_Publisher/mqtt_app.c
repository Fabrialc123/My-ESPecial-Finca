/* CREATED BY FABRIZIO ALCARAZ ESCOBAR */

/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

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
#include "mqtt_app.h"
#include "task_common.h"
#include "recollecter.h"

static const char TAG[] = "MQTT_APP";

static QueueHandle_t mqtt_app_queue_handle;
static esp_mqtt_client_handle_t client;

static int sended_packs;
static int received_packs;

static pthread_mutex_t mutex_MQTT_APP_MQTT_CONNECTED;
static int MQTT_CONNECTED = 0;
static int s_retry_num = 0;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_app_disconnect(void){
	int status;

	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
	MQTT_CONNECTED = 0;
	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

	status = esp_mqtt_client_disconnect(client);
	ESP_LOGE(TAG,"Client disconnected (%d)", status);
	status = esp_mqtt_client_stop(client);
	ESP_LOGE(TAG,"Client stopped (%d)", status);
	status = esp_mqtt_client_destroy(client);
	ESP_LOGE(TAG,"Client destroyed (%d)", status);

	vQueueDelete(mqtt_app_queue_handle);

}

static void mqtt_app_data_sender (void *pvParameters){
	 ESP_LOGE("MQTT_APP_DATA_SENDER","STACK SIZE: %d / %d",uxTaskGetStackHighWaterMark(NULL), MQTT_APP_DATA_SENDER_STACK_SIZE);
	 ESP_LOGI("MQTT_APP_DATA_SENDER", "Sending data every %d seconds.", MQTT_APP_TIME_TO_SEND_DATA/100);
	 vTaskDelay(MQTT_APP_TIME_TO_SEND_DATA);
	 for(;;){
		 ESP_LOGI("MQTT_APP_DATA_SENDER","Sending data...");
		 mqtt_app_send_message(MQTT_APP_MSG_SEND_DATA, MQTT_APP_GENERAL_TOPIC);
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
	char desc[MQTT_APP_TOPIC_LENGTH];
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

    	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
    	MQTT_CONNECTED = 1;
    	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

       msg_id = esp_mqtt_client_subscribe(client, MQTT_APP_GENERAL_TOPIC, 0);
       ESP_LOGE(TAG, "sent subscribe successful to topic %s , msg_id=%d (FOR TEST ONLY!)",MQTT_APP_GENERAL_TOPIC, msg_id);

       msg_id = esp_mqtt_client_subscribe(client, MQTT_APP_BROKER_TOPIC, 0);
       ESP_LOGI(TAG, "sent subscribe successful to topic %s , msg_id=%d",MQTT_APP_BROKER_TOPIC, msg_id);

       xTaskCreatePinnedToCore(&mqtt_app_data_sender, "mqtt_app_data_sender", MQTT_APP_DATA_SENDER_STACK_SIZE, NULL, MQTT_APP_DATA_SENDER_PRIORITY, NULL, MQTT_APP_DATA_SENDER_CORE_ID);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        if (s_retry_num >= MQTT_APP_MAX_CONNECTION_RETRIES) {
        	ESP_LOGE(TAG, "Max connection retries, disconnecting and setting down MQTT!");
        	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
        	MQTT_CONNECTED = -1;
        	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

        }else {
			ESP_LOGE(TAG, "Retrying reconnect... (%d/%d)", ++s_retry_num, MQTT_APP_MAX_CONNECTION_RETRIES);
			esp_mqtt_client_reconnect(client);
        }

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
        ESP_LOGI(TAG, "PACKS RECEIVED: %d",++received_packs);

        if(strncmp(event->topic,MQTT_APP_BROKER_TOPIC,event->topic_len) == 0){
        	ESP_LOGI(TAG,"Received a refresh request from topic %.*s to topic %.*s",event->topic_len,event->topic,event->data_len,event->data);
        	memset(&desc, 0, MQTT_APP_TOPIC_LENGTH);
        	strncpy(desc,event->data,event->data_len);
        	mqtt_app_send_message(MQTT_APP_MSG_SEND_DATA,desc);
        }

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
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


void mqtt_app_send_data(char topic[MQTT_APP_TOPIC_LENGTH]){
	int msg_id, i, recollecters ,len;
	char data[MAX_STRING_LENGTH];
	recollecters = get_recollecters_size();
	for (i = 0; i < recollecters;i++){
		memset(&data, 0, MAX_STRING_LENGTH);
		len = get_sensor_data (i, &data);
		if (len < 0 || len >= MAX_STRING_LENGTH){
			ESP_LOGE(TAG, "ERROR in get_sensor_data(sensor_id = %d)", i);
			return;
		}
		//ESP_LOGI(TAG, "Data to send: %s", data);
		ESP_LOGE(TAG, "TESTING DATA SIZE (%d/%d)",strlen(data),len);
		msg_id = esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d, PACKS SENDED: %d", msg_id, ++sended_packs);
	}
}

static void mqtt_app_task(void *pvParameters)
{
	mqtt_app_queue_msg_t msg;
	/*
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "broker.hivemq.com",
    };
*/
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_APP_URI,
		.port = MQTT_APP_PORT,
    };

    ESP_LOGE(TAG,"STACK SIZE: %d / %d",uxTaskGetStackHighWaterMark(NULL), MQTT_APP_TASK_STACK_SIZE);

    //esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

	for(;;){
		if (xQueueReceive(mqtt_app_queue_handle, &msg, portMAX_DELAY)){
			switch(msg.msgID){
				case MQTT_APP_MSG_SEND_DATA:
					ESP_LOGI(TAG, "MQTT_APP_MSG_SEND_DATA to topic %s",msg.desc);

					mqtt_app_send_data(msg.desc);

					break;

				case MQTT_APP_MSG_SUBSCRIBE:
					ESP_LOGI(TAG, "MQTT_APP_MSG_SUBSCRIBE to topic %s", msg.desc);

				    esp_mqtt_client_subscribe(client, msg.desc, 0);
				    ESP_LOGI(TAG, "sent subscribe to topic %s ",msg.desc);

					break;

				default:
					break;
			}
		}
	}
}


int is_mqtt_connected(){
	int aux;

	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
	aux = MQTT_CONNECTED;
	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

	return aux;
}

BaseType_t mqtt_app_send_message(mqtt_app_msg_e msgID, char desc[CHAR_LENGTH]){
	mqtt_app_queue_msg_t msg;

	msg.msgID = msgID;
	strcpy(msg.desc,desc);

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

    mqtt_app_queue_handle = xQueueCreate(3, sizeof(mqtt_app_queue_msg_t));

    xTaskCreatePinnedToCore(&mqtt_app_task, "mqtt_app_task", MQTT_APP_TASK_STACK_SIZE, NULL, MQTT_APP_TASK_PRIORITY, NULL, MQTT_APP_TASK_CORE_ID);

    ESP_LOGI(TAG,"Waiting for mqtt broker connection");
    while(!is_mqtt_connected()){
    	vTaskDelay(10);
    }

    if (is_mqtt_connected() == -1) mqtt_app_disconnect();

}

void mqtt_app_refresh_TEST(void){
	ESP_LOGE(TAG,"TESTING REFRESH FUNCTION");
	esp_mqtt_client_subscribe(client,"/REFRESHTESTING", 0);
	vTaskDelay(1500);
	ESP_LOGE(TAG,"TESTING REFRESH REQUEST");
	esp_mqtt_client_publish(client, MQTT_APP_BROKER_TOPIC, "/REFRESHTESTING", 0, 1, 0);
	while(1){
		vTaskDelay(1500);
		esp_mqtt_client_publish(client, MQTT_APP_BROKER_TOPIC, "/REFRESHTESTING", 0, 1, 0);
	}
}
