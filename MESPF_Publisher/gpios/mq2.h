/*
 * mq2.h
 *
 *  Created on: 11 oct. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_MQ2_H_
#define MAIN_GPIOS_MQ2_H_

#include "driver/adc.h"

#define N_SAMPLES   	64          						//Used for minimizing noise

static const adc1_channel_t channel = ADC1_CHANNEL_6;     	//ADC1 channel 6 -> GPIO34
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_0;

/*
 * Initializes MQ2 peripheral
 */
void mq2_init(void);

/**
 * Gets the converted data
 */
int mq2_get_data(void);


#endif /* MAIN_GPIOS_MQ2_H_ */
