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

static const char TAG[] = "MQTT_APP";

static QueueHandle_t mqtt_app_queue_handle;
static esp_mqtt_client_handle_t client;

static pthread_mutex_t mutex_MQTT_APP_RECOLLECTER;
static mqtt_app_foo_recollecter *recollecters;
static int recollecters_n;

static int sended_packs;
static int received_packs;


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
//	int msg_id;
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

//        msg_id = esp_mqtt_client_subscribe(client, "/TESTFABRI", 0);
//        ESP_LOGI(TAG, "sent subscribe successful to topic /TESTFABRI , msg_id=%d (FOR TEST ONLY!)", msg_id);

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

void mqtt_app_register_recollector (mqtt_app_foo_recollecter rtr){
	pthread_mutex_lock(&mutex_MQTT_APP_RECOLLECTER);
	recollecters[recollecters_n++] = rtr;
	pthread_mutex_unlock(&mutex_MQTT_APP_RECOLLECTER);
}

void mqtt_app_recollect_data (void){
	int i;
	pthread_mutex_lock(&mutex_MQTT_APP_RECOLLECTER);
	for(i = 0; i < recollecters_n;i++){
		data_to_send[i] = recollecters[i]();
	}
	pthread_mutex_unlock(&mutex_MQTT_APP_RECOLLECTER);
}

void mqtt_app_send_data(void){
	int msg_id, i;
	char data[] = "";

	pthread_mutex_lock(&mutex_MQTT_APP_RECOLLECTER);
	for(i = 0; i < recollecters_n; i++){
		strcat(data, data_to_send[i].sensorName);
	}


	msg_id = esp_mqtt_client_publish(client, "/TESTFABRI", data, 0, 1, 0);
	ESP_LOGI(TAG, "sent publish successful, msg_id=%d, PACKS SENDED: %d", msg_id, ++sended_packs);

	pthread_mutex_unlock(&mutex_MQTT_APP_RECOLLECTER);

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
        .uri = MQTT_URI,
		.port = MQTT_PORT,
    };

    ESP_LOGE(TAG,"STACK SIZE: %d",uxTaskGetStackHighWaterMark(NULL));

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

					mqtt_app_recollect_data();
					mqtt_app_send_data();

					break;

				default:
					break;
			}
		}
	}
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

	if(pthread_mutex_init (&mutex_MQTT_APP_RECOLLECTER, NULL) != 0){
	 ESP_LOGE(TAG,"Failed to initialize the recollecter mutex");
	}

	sended_packs = 0;
	received_packs = 0;

    recollecters = (mqtt_app_foo_recollecter *)malloc(sizeof(mqtt_app_foo_recollecter) * MQTT_APP_SENSOR_DATA_SIZE);
    recollecters_n = 0;

    mqtt_app_queue_handle = xQueueCreate(3, sizeof(mqtt_app_queue_msg_t));

    xTaskCreatePinnedToCore(&mqtt_app_task, "mqtt_app_task", MQTT_APP_TASK_STACK_SIZE, NULL, MQTT_APP_TASK_PRIORITY, NULL, MQTT_APP_TASK_CORE_ID);
}
