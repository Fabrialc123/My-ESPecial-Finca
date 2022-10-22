/*
 * dht22.c
 *
 *  Created on: 19 oct. 2022
 *      Author: Kike
 */

#include "dht22.h"

#include <stdbool.h>

#include "unistd.h"
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"

static const char TAG[]  = "dht22";

bool g_dht22_initialized = false;

static sensor_data_t sensor_data;

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

void dht22_init(void){

	int number_of_values = 2;

	strcpy(sensor_data.sensorName, "DHT22");

	sensor_data.valuesLen = number_of_values;

	sensor_data.sensor_values = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(sensor_data.sensor_values[0].valueName, "Hum");
	sensor_data.sensor_values[0].sensor_value_type = FLOAT;

	strcpy(sensor_data.sensor_values[1].valueName, "Temp");
	sensor_data.sensor_values[1].sensor_value_type = FLOAT;

	g_dht22_initialized = true;
}

sensor_data_t dht22_get_sensor_data(void){

	int uSec;

	uint8_t dhtData[DHT22_MAX_DATA];
	uint8_t byteInx = 0;
	uint8_t bitInx = 7;

	float humidity = 0.0;
	float temperature = 0.0;

	sensor_data.sensor_values[0].sensor_value.fval = 0.0;
	sensor_data.sensor_values[1].sensor_value.fval = 0.0;

	if(g_dht22_initialized){

		for (int k = 0; k < DHT22_MAX_DATA; k++)
			dhtData[k] = 0;

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
	//	ESP_LOGI( TAG, "Response = %d", uSec );
		if(uSec < 0){
			return sensor_data;
		}

		uSec = dht22_get_signal_level(82, 1);
	//	ESP_LOGI( TAG, "Response = %d", uSec );
		if(uSec < 0){
			return sensor_data;
		}

		// No errors, read the 40 data bits
		for(int k = 0; k < 40; k++){

			// Starts new data transmission with >50us low signal
			uSec = dht22_get_signal_level(52, 0);
			if(uSec < 0){
				return sensor_data;
			}

			// Check to see if after >70us RX data is a 0 or a 1
			uSec = dht22_get_signal_level(72, 1);
			if(uSec < 0){
				return sensor_data;
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
		humidity = dhtData[0];
		humidity *= 0x100;					// >> 8
		humidity += dhtData[1];
		humidity /= 10;						// get the decimal

		// Get temperature from Data[2] and Data[3]
		temperature = dhtData[2] & 0x7F;
		temperature *= 0x100;				// >> 8
		temperature += dhtData[3];
		temperature /= 10;					// get the decimal

		if(dhtData[2] & 0x80) 				// negative temperature
			temperature *= -1;

		// Verify if checksum is OK
		// Checksum is the sum of Data 8 bits masked out 0xFF
		if (dhtData[4] == ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF)){
			sensor_data.sensor_values[0].sensor_value.fval = humidity;
			sensor_data.sensor_values[1].sensor_value.fval = temperature;
		}

	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
	}
	
	return sensor_data;
}
