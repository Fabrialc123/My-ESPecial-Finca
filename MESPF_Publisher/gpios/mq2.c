/*
 * mq2.c
 *
 *  Created on: 11 oct. 2022
 *      Author: Kike
 */

#include "gpios/mq2.h"

#include <stdbool.h>

#include "esp_log.h"

static const char TAG[] = "mq2";

bool g_mq2_initialized 	= false;

void mq2_init(void){

	adc1_config_width(width);
	adc1_config_channel_atten(channel, atten);

	g_mq2_initialized = true;
}

int mq2_get_data(void){

	int read_raw = 0;

	if(g_mq2_initialized){

        for (int i = 0; i < N_SAMPLES; i++) {
        	read_raw += adc1_get_raw(channel);
        }

        read_raw /= N_SAMPLES;
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the MQ2 without initializing the ADC channel");
	}

	return read_raw;
}
