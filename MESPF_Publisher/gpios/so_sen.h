/*
 * so_sen.h
 *
 *  Created on: 4 nov. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_SO_SEN_H_
#define MAIN_GPIOS_SO_SEN_H_

#include "driver/adc.h"
#include "recollecter.h"

#define N_SAMPLES   	64          						//Used for minimizing noise

static const adc1_channel_t channel_so_sen = ADC1_CHANNEL_7;     	//ADC1 channel 7 -> GPIO35
static const adc_bits_width_t width_so_sen = ADC_WIDTH_BIT_12;
static const adc_atten_t atten_so_sen = ADC_ATTEN_DB_11;

#define SO_SEN_TIME_TO_UPDATE_DATA	100 * 5 				// 1 second = 100 ticks

/*
 * Initializes SO_SEN peripheral
 */
void so_sen_init(void);

/**
 * Gets data
 */
sensor_data_t so_sen_get_sensor_data(void);

#endif /* MAIN_GPIOS_SO_SEN_H_ */
