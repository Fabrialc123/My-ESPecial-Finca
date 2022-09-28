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
static int MQTT_CONNECTED;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
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
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

    	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
    	MQTT_CONNECTED = 1;
    	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

       msg_id = esp_mqtt_client_subscribe(client, MQTT_APP_TOPIC, 0);
       ESP_LOGI(TAG, "sent subscribe successful to topic %s , msg_id=%d (FOR TEST ONLY!)",MQTT_APP_TOPIC, msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


void mqtt_app_send_data(void){
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
		msg_id = esp_mqtt_client_publish(client, MQTT_APP_TOPIC, data, 0, 1, 0);
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
					ESP_LOGI(TAG, "MQTT_APP_MSG_SEND_DATA");

					mqtt_app_send_data();

					break;

				default:
					break;
			}
		}
	}
}

static void mqtt_app_data_sender (void *pvParameters){
	 ESP_LOGE("MQTT_APP_DATA_SENDER","STACK SIZE: %d / %d",uxTaskGetStackHighWaterMark(NULL), MQTT_APP_DATA_SENDER_STACK_SIZE);
	 ESP_LOGI("MQTT_APP_DATA_SENDER", "Sending data every %d seconds.", MQTT_APP_TIME_TO_SEND_DATA/100);

	 for(;;){
		 ESP_LOGI("MQTT_APP_DATA_SENDER","Sending data...");
		 mqtt_app_send_message(MQTT_APP_MSG_SEND_DATA);
		 vTaskDelay(MQTT_APP_TIME_TO_SEND_DATA);
	 }
}

int is_mqtt_connected(){
	int aux;

	pthread_mutex_lock(&mutex_MQTT_APP_MQTT_CONNECTED);
	aux = MQTT_CONNECTED;
	pthread_mutex_unlock(&mutex_MQTT_APP_MQTT_CONNECTED);

	return aux;
}

BaseType_t mqtt_app_send_message(mqtt_app_msg_e msgID){
	mqtt_app_queue_msg_t msg;

	msg.msgID = msgID;

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

	MQTT_CONNECTED = 0;

    mqtt_app_queue_handle = xQueueCreate(3, sizeof(mqtt_app_queue_msg_t));

    xTaskCreatePinnedToCore(&mqtt_app_task, "mqtt_app_task", MQTT_APP_TASK_STACK_SIZE, NULL, MQTT_APP_TASK_PRIORITY, NULL, MQTT_APP_TASK_CORE_ID);

    while(!is_mqtt_connected());

    xTaskCreatePinnedToCore(&mqtt_app_data_sender, "mqtt_app_data_sender", MQTT_APP_DATA_SENDER_STACK_SIZE, NULL, MQTT_APP_DATA_SENDER_PRIORITY, NULL, MQTT_APP_DATA_SENDER_CORE_ID);

}
