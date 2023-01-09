/*
 * dht22.c
 *
 *  Created on: 19 oct. 2022
 *      Author: Kike
 */

#include "gpios/dht22.h"
#include "task_common.h"

#include <stdbool.h>
#include <pthread.h>

#include "unistd.h"
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"

static const char TAG[]  = "dht22";

bool g_dht22_initialized = false;

static float humidity;
static float temperature;

static pthread_mutex_t mutex_dht22;

/**
 * Function to check income signal level
 * @param usTimeOut, max microseconds to be considered communication error
 * @param state, expected income signal level
 * @return number of microseconds passed in the expected state, -1 if timeout occurs
 */
static int dht22_get_signal_level(int usTimeOut, bool state){

	int uSec = 0;
	
	while(gpio_get_level(DHT22_GPIO) == state){

		if(uSec > usTimeOut)
			return -1;
		
		uSec++;
		usleep(1);
	}
	
	return uSec;
}

/**
 * Task for DHT22
 */

static void dht22_task(void *pvParameters){

	int uSec;

	uint8_t dhtData[DHT22_MAX_DATA];
	uint8_t byteInx;
	uint8_t bitInx;

	bool failed;

	float auxHumidity;
	float auxTemperature;

	for(;;){

		pthread_mutex_lock(&mutex_dht22);

		for (int k = 0; k < DHT22_MAX_DATA; k++)
			dhtData[k] = 0;

		byteInx = 0;
		bitInx = 7;

		failed = false;

		// Send start signal to DHT sensor
		gpio_set_direction(DHT22_GPIO, GPIO_MODE_OUTPUT);

		// Pull down for 3 ms for a smooth and nice wake up
		gpio_set_level(DHT22_GPIO, 0);
		usleep(3000);

		// Pull up for 25 us for a gentile asking for data
		gpio_set_level(DHT22_GPIO, 1);
		usleep(25);

		gpio_set_direction(DHT22_GPIO, GPIO_MODE_INPUT);	// change to input mode

		// DHT will keep the line low for 80 us and then high for 80 us
		uSec = dht22_get_signal_level(82, 0);
		if(uSec < 0){
			failed = true;
		}

		uSec = dht22_get_signal_level(82, 1);
		if(uSec < 0){
			failed = true;
		}

		// No errors, read the 40 data bits
		for(int k = 0; k < 40; k++){

			// Starts new data transmission with >50us low signal
			uSec = dht22_get_signal_level(52, 0);
			if(uSec < 0){
				failed = true;
			}

			// Check to see if after >70us RX data is a 0 or a 1
			uSec = dht22_get_signal_level(72, 1);
			if(uSec < 0){
				failed = true;
			}

			// Add the current read to the output data.
			// Since all dhtData array where set to 0 at the start,
			// only look for "1" (>28us us)
			if (uSec > 40){
				dhtData[byteInx] |= (1 << bitInx);
			}

			// Index to next byte
			if (bitInx == 0){
				bitInx = 7;
				byteInx++;
			}
			else{
				bitInx--;
			}
		}

		// Get humidity from Data[0] and Data[1]
		auxHumidity = dhtData[0];
		auxHumidity *= 0x100;					// >> 8
		auxHumidity += dhtData[1];
		auxHumidity /= 10;						// get the decimal

		// Get temperature from Data[2] and Data[3]
		auxTemperature = dhtData[2] & 0x7F;
		auxTemperature *= 0x100;				// >> 8
		auxTemperature += dhtData[3];
		auxTemperature /= 10;					// get the decimal

		if(dhtData[2] & 0x80) 				// negative temperature
			auxTemperature *= -1;

		// Verify if checksum is OK
		// Checksum is the sum of Data 8 bits masked out 0xFF
		if ((dhtData[4] == ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF)) && !failed){
			humidity = auxHumidity;
			temperature = auxTemperature;
		}

		pthread_mutex_unlock(&mutex_dht22);

		vTaskDelay(DHT22_TIME_TO_UPDATE_DATA);
	}
}

void dht22_init(void){

	if(pthread_mutex_init(&mutex_dht22, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the DHT22 mutex");
	}
	else{
		int res;

		res = register_recollecter(&dht22_get_sensor_data);

		if(res == 1){
			ESP_LOGI(TAG, "DHT22 recollecter successfully registered");
			
			humidity = 0;
			temperature = 0;

			xTaskCreatePinnedToCore(&dht22_task, "dht22_task", DHT22_STACK_SIZE, NULL, DHT22_PRIORITY, NULL, DHT22_CORE_ID);

			g_dht22_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, DHT22 recollecter hasn't been registered");
		}
	}

}

sensor_data_t dht22_get_sensor_data(void){

	sensor_data_t aux;
	sensor_value_t *aux2;
	int number_of_values = 2;

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		aux.valuesLen = 0;
		return aux;
	}

	pthread_mutex_lock(&mutex_dht22);

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "DHT22");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;

	strcpy(aux.sensor_values[0].valueName,"Humidity");
	aux.sensor_values[0].sensor_value_type = FLOAT;
	aux.sensor_values[0].sensor_value.fval = humidity;

	strcpy(aux.sensor_values[1].valueName,"Temperature");
	aux.sensor_values[1].sensor_value_type = FLOAT;
	aux.sensor_values[1].sensor_value.fval = temperature;
	
	pthread_mutex_unlock(&mutex_dht22);

	return aux;
}
