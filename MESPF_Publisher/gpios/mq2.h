/*
 * mq2.h
 *
 *  Created on: 11 oct. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_MQ2_H_
#define MAIN_GPIOS_MQ2_H_

#include "driver/adc.h"
#include "recollecter.h"

#define MQ2_LOW_V		1000

#define MQ2_HIGH_V		4095

#define N_SAMPLES   	64          						//Used for minimizing noise

static const adc1_channel_t channel_mq2 = ADC1_CHANNEL_6;     	//ADC1 channel 6 -> GPIO34
static const adc_bits_width_t width_mq2 = ADC_WIDTH_BIT_12;
static const adc_atten_t atten_mq2 = ADC_ATTEN_DB_11;

#define MQ2_TIME_TO_UPDATE_DATA	100 						// 1 second = 100 ticks

// Value related to the LCD

#define MQ2_SHOW_SMOKE_GAS_PERCENTAGE_ON_LCD	true

// Values related to the alerts

#define MQ2_ALERT_SMOKE_GAS						true
#define MQ2_SMOKE_GAS_TICKS_TO_ALERT			3
#define MQ2_SMOKE_GAS_UPPER_THRESHOLD			70.0
#define MQ2_SMOKE_GAS_LOWER_THRESHOLD			0.0

/*
 * Initializes MQ2 peripheral
 */
void mq2_init(void);

/**
 * Gets data
 */
sensor_data_t mq2_get_sensor_data(void);


#endif /* MAIN_GPIOS_MQ2_H_ */
