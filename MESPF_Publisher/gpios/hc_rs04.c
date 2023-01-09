/*
 * hc_rs04.c
 *
 *  Created on: 3 nov. 2022
 *      Author: Kike
 */

#include "gpios/hc_rs04.h"
#include "task_common.h"

#include <stdbool.h>
#include <pthread.h>

#include "unistd.h"
#include "esp_log.h"
#include "string.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"
#include "driver/gpio.h"

static const char TAG[]  = "hc_rs04";

bool g_hc_rs04_initialized = false;

static float water_level_percentage;

static pthread_mutex_t mutex_hc_rs04;

static xQueueHandle cap_queue;

/**
 * This is an ISR callback, we take action according to the captured edge
 */

static bool hc_rs04_echo_isr_handler(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_sig, const cap_event_data_t *edata, void *arg) {
	static uint32_t cap_val_begin_of_sample = 0;
	static uint32_t cap_val_end_of_sample = 0;

    //calculate the interval in the ISR,
    //so that the interval will be always correct even when cap_queue is not handled in time and overflow.
    BaseType_t high_task_wakeup = pdFALSE;

    if(edata->cap_edge == MCPWM_POS_EDGE){
        // store the timestamp when pos edge is detected
        cap_val_begin_of_sample = edata->cap_value;
        cap_val_end_of_sample = cap_val_begin_of_sample;
    }
    else{
        cap_val_end_of_sample = edata->cap_value;

        uint32_t pulse_count = cap_val_end_of_sample - cap_val_begin_of_sample;

        // send measurement back though queue
        xQueueSendFromISR(cap_queue, &pulse_count, &high_task_wakeup);
    }

    return high_task_wakeup == pdTRUE;
}

/**
 * Task for HC_RS04
 */

static void hc_rs04_task(void *pvParameters){

	uint32_t pulse_count;
	uint32_t pulse_width_us;
	float distance;

	for(;;){

		// Pull up trig pin for 10 us
		gpio_set_level(HC_RS04_TRIG_GPIO, 1);
		usleep(10);

		// Pull it down now
		gpio_set_level(HC_RS04_TRIG_GPIO, 0);

		xQueueReceive(cap_queue, &pulse_count, portMAX_DELAY);

		pthread_mutex_lock(&mutex_hc_rs04);

		pulse_width_us = pulse_count * (1000000.0 / rtc_clk_apb_freq_get());

        if (pulse_width_us > 35000) {
        	ESP_LOGE(TAG,"Error, distance cannot be measured ");
        }
        else{
        	distance = (float) pulse_width_us / 59;

        	water_level_percentage = 100 - (((distance - DISTANCE_BETWEEN_SENSOR_AND_TANK) / TANK_LENGTH) * 100);
        }

		pthread_mutex_unlock(&mutex_hc_rs04);

		vTaskDelay(HC_RS04_TIME_TO_UPDATE_DATA);
	}
}

void hc_rs04_init(void){

	if(pthread_mutex_init(&mutex_hc_rs04, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the HC_RS04 mutex");
	}
	else{
		int res;

		res = register_recollecter(&hc_rs04_get_sensor_data);

		if(res == 1){
			ESP_LOGI(TAG, "HC_RS04 recollecter successfully registered");

		    // Create the queue
		    cap_queue = xQueueCreate(1, sizeof(uint32_t));

		    if (cap_queue == NULL) {
		        ESP_LOGE(TAG, "Failed creating the message queue");
		    }
		    else{
				water_level_percentage = 0;

				// Set echo pin
				mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, HC_RS04_ECHO_GPIO);
				gpio_pulldown_en(HC_RS04_ECHO_GPIO);

			    mcpwm_capture_config_t conf = {
			        .cap_edge = MCPWM_BOTH_EDGE,
			        .cap_prescale = 1,
			        .capture_cb = hc_rs04_echo_isr_handler,
			        .user_data = NULL
			    };

			    mcpwm_capture_enable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, &conf);

				// Set trig pin
			    gpio_config_t io_conf = {
			        .intr_type = GPIO_INTR_DISABLE,
			        .mode = GPIO_MODE_OUTPUT,
			        .pull_down_en = GPIO_PULLDOWN_DISABLE,
			        .pull_up_en = GPIO_PULLUP_DISABLE,
			        .pin_bit_mask = BIT64(HC_RS04_TRIG_GPIO),
			    };

			    gpio_config(&io_conf);
			    gpio_set_level(HC_RS04_TRIG_GPIO, 0);


				xTaskCreatePinnedToCore(&hc_rs04_task, "hc_rs04_task", HC_RS04_STACK_SIZE, NULL, HC_RS04_PRIORITY, NULL, HC_RS04_CORE_ID);

				g_hc_rs04_initialized = true;
		    }
		}
		else{
			ESP_LOGE(TAG, "Error, HC_RS04 recollecter hasn't been registered");
		}
	}
}

sensor_data_t hc_rs04_get_sensor_data(void){

	sensor_data_t aux;
	sensor_value_t *aux2;
	int number_of_values = 1;

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC_RS04 without initializing it");
		aux.valuesLen = 0;
		return aux;
	}

	pthread_mutex_lock(&mutex_hc_rs04);

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "HC-RS04");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;

	strcpy(aux.sensor_values[0].valueName,"Water level");
	aux.sensor_values[0].sensor_value_type = FLOAT;
	aux.sensor_values[0].sensor_value.fval = water_level_percentage;

	pthread_mutex_unlock(&mutex_hc_rs04);

	return aux;
}
