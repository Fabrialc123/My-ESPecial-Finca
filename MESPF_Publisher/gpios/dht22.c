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
#include <nvs_app.h>

#include "unistd.h"
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "mqtt/mqtt_commands.h"
#include "gpios_manager.h"
#include "sensors_manager.h"

// Values related to the DHT22 sensor in general

static TaskHandle_t task_dht22 					= NULL;

static const char TAG[]  						= "dht22";

bool g_dht22_initialized 						= false;

static pthread_mutex_t mutex_dht22;

// Arrays for the DHT22 sensors management

static dht22_gpios_t* dht22_gpios_array;
static dht22_data_t* dht22_data_array;

static int dht22_cont							= 0;

// Values related to the alerts

	// 0: Humidity
static bool dht22_alert_humidity				= false;
static int dht22_humidity_ticks_to_alert		= 5;
static float dht22_humidity_upper_threshold		= 1000.0;
static float dht22_humidity_lower_threshold		= -1000.0;

static int* humidity_alert_counter;
static bool* humidity_is_alerted;

	// 1: Temperature
static bool dht22_alert_temperature				= false;
static int dht22_temperature_ticks_to_alert		= 5;
static float dht22_temperature_upper_threshold	= 1000.0;
static float dht22_temperature_lower_threshold	= -1000.0;

static int* temperature_alert_counter;
static bool* temperature_is_alerted;

// NVS keys

#define nvs_DHT22_CONT_key		"dht22_cont"
#define nvs_DHT22_GPIOS_key		"dht22_gpios"

/**
 * Shift and delete
 */
static void shift_delete(int pos){

	for(int i = pos; i < dht22_cont - 1; i++){

		dht22_gpios_array[pos].data = dht22_gpios_array[pos + 1].data;

		dht22_data_array[pos].humidity = dht22_data_array[pos + 1].humidity;
		dht22_data_array[pos].temperature = dht22_data_array[pos + 1].temperature;

		humidity_alert_counter[pos] = humidity_alert_counter[pos + 1];
		humidity_is_alerted[pos] = humidity_is_alerted[pos + 1];

		temperature_alert_counter[pos] = temperature_alert_counter[pos + 1];
		temperature_is_alerted[pos] = temperature_is_alerted[pos + 1];
	}

	dht22_cont--;
}

/**
 * Function to check income signal level
 * @param gpio, gpio to be checked
 * @param usTimeOut, max microseconds to be considered communication error
 * @param state, expected income signal level
 * @return number of microseconds passed in the expected state, -1 if timeout occurs
 */
static int dht22_get_signal_level(int gpio, int usTimeOut, bool state){

	int uSec = 0;
	
	while(gpio_get_level(gpio) == state){

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

	int msg_id = 0;

	for(;;){

		pthread_mutex_lock(&mutex_dht22);

		for(int x = 0; x < dht22_cont; x++){

			for (int k = 0; k < DHT22_MAX_DATA; k++)
				dhtData[k] = 0;

			byteInx = 0;
			bitInx = 7;

			failed = false;

			// Send start signal to DHT sensor
			gpio_set_direction(dht22_gpios_array[x].data, GPIO_MODE_OUTPUT);

			// Pull down for 3 ms for a smooth and nice wake up
			gpio_set_level(dht22_gpios_array[x].data, 0);
			usleep(3000);

			// Pull up for 25 us for a gentile asking for data
			gpio_set_level(dht22_gpios_array[x].data, 1);
			usleep(25);

			gpio_set_direction(dht22_gpios_array[x].data, GPIO_MODE_INPUT);	// change to input mode

			// DHT will keep the line low for 80 us and then high for 80 us
			uSec = dht22_get_signal_level(dht22_gpios_array[x].data, 82, 0);
			if(uSec < 0){
				failed = true;
			}

			uSec = dht22_get_signal_level(dht22_gpios_array[x].data, 82, 1);
			if(uSec < 0){
				failed = true;
			}

			// No errors, read the 40 data bits
			for(int k = 0; k < 40; k++){

				// Starts new data transmission with >50us low signal
				uSec = dht22_get_signal_level(dht22_gpios_array[x].data, 52, 0);
				if(uSec < 0){
					failed = true;
				}

				// Check to see if after >70us RX data is a 0 or a 1
				uSec = dht22_get_signal_level(dht22_gpios_array[x].data, 72, 1);
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
				dht22_data_array[x].humidity = auxHumidity;
				dht22_data_array[x].temperature = auxTemperature;
			}

			// Humidity alert
			if(dht22_alert_humidity){

				// Update counter
				if((dht22_data_array[x].humidity < dht22_humidity_lower_threshold) && (humidity_alert_counter[x] > -dht22_humidity_ticks_to_alert))
					humidity_alert_counter[x]--;
				else if((dht22_data_array[x].humidity > dht22_humidity_upper_threshold) && (humidity_alert_counter[x] < dht22_humidity_ticks_to_alert))
					humidity_alert_counter[x]++;
				else{
					if(humidity_alert_counter[x] > 0)
						humidity_alert_counter[x]--;
					else if(humidity_alert_counter[x] < 0)
						humidity_alert_counter[x]++;
				}

				// Check if the value can be alerted again
				if((humidity_alert_counter[x] == 0) && humidity_is_alerted[x]){
					humidity_is_alerted[x] = false;
				}

				// Check if it is the moment to alert
				if((humidity_alert_counter[x] == -dht22_humidity_ticks_to_alert) && !humidity_is_alerted[x]){

					mqtt_app_send_alert("DHT22", msg_id, "WARNING in Sensor DHT22! exceed on lower threshold (value: humidity)");

					humidity_is_alerted[x] = true;
					msg_id++;
				}
				else if((humidity_alert_counter[x] == dht22_humidity_ticks_to_alert) && !humidity_is_alerted[x]){

					mqtt_app_send_alert("DHT22", msg_id, "WARNING in Sensor DHT22! exceed on upper threshold (value: humidity)");

					humidity_is_alerted[x] = true;
					msg_id++;
				}
			}

			// Temperature alert
			if(dht22_alert_temperature){

				// Update counter
				if((dht22_data_array[x].temperature < dht22_temperature_lower_threshold) && (temperature_alert_counter[x] > -dht22_temperature_ticks_to_alert))
					temperature_alert_counter[x]--;
				else if((dht22_data_array[x].temperature > dht22_temperature_upper_threshold) && (temperature_alert_counter[x] < dht22_temperature_ticks_to_alert))
					temperature_alert_counter[x]++;
				else{
					if(temperature_alert_counter[x] > 0)
						temperature_alert_counter[x]--;
					else if(temperature_alert_counter[x] < 0)
						temperature_alert_counter[x]++;
				}

				// Check if the value can be alerted again
				if((temperature_alert_counter[x] == 0) && temperature_is_alerted[x]){
					temperature_is_alerted[x] = false;
				}

				// Check if it is the moment to alert
				if((temperature_alert_counter[x] == -dht22_temperature_ticks_to_alert) && !temperature_is_alerted[x]){

					mqtt_app_send_alert("DHT22", msg_id, "WARNING in Sensor DHT22! exceed on lower threshold (value: temperature)");

					temperature_is_alerted[x] = true;
					msg_id++;
				}
				else if((temperature_alert_counter[x] == dht22_temperature_ticks_to_alert) && !temperature_is_alerted[x]){

					mqtt_app_send_alert("DHT22", msg_id, "WARNING in Sensor DHT22! exceed on upper threshold (value: temperature)");

					temperature_is_alerted[x] = true;
					msg_id++;
				}
			}
		}

		pthread_mutex_unlock(&mutex_dht22);

		vTaskDelay(DHT22_TIME_TO_UPDATE_DATA);
	}
}

void dht22_startup(void){
	size_t size = 0;
	uint8_t aux = 0;

	nvs_app_get_uint8_value(nvs_DHT22_CONT_key,&aux);

	dht22_cont = aux;

	dht22_gpios_array = (dht22_gpios_t*) malloc(sizeof(dht22_gpios_t) * dht22_cont);
	dht22_data_array = (dht22_data_t*) malloc(sizeof(dht22_data_t) * dht22_cont);

	humidity_alert_counter = (int*) malloc(sizeof(int) * dht22_cont);
	humidity_is_alerted = (bool*) malloc(sizeof(bool) * dht22_cont);

	temperature_alert_counter = (int*) malloc(sizeof(int) * dht22_cont);
	temperature_is_alerted = (bool*) malloc(sizeof(bool) * dht22_cont);

	if(dht22_cont > 0){
		nvs_app_get_blob_value(nvs_DHT22_GPIOS_key,NULL,&size);
		if(size != 0)
			nvs_app_get_blob_value(nvs_DHT22_GPIOS_key,dht22_gpios_array,&size);

		int gpios[DHT22_N_GPIOS];
		for(int i = 0; i < dht22_cont; i++){
			gpios[0] = dht22_gpios_array[i].data;
			gpios_manager_lock(gpios, DHT22_N_GPIOS);
		}

		dht22_init();
	}
}

void dht22_init(void){

	if(pthread_mutex_init(&mutex_dht22, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the DHT22 mutex");
	}
	else{
		int res_rec, res_sen;

		res_rec = register_recollecter(&dht22_get_sensors_data, &dht22_get_sensors_gpios, &dht22_get_sensors_additional_parameters);
		res_sen = sensors_manager_add(&dht22_destroy, &dht22_add_sensor, &dht22_delete_sensor, &dht22_set_gpios, &dht22_set_parameters, &dht22_set_alert_values);

		if(res_rec == 1 && res_sen == 1){
			ESP_LOGI(TAG, "DHT22 recollecter successfully registered");

			xTaskCreatePinnedToCore(&dht22_task, "dht22_task", DHT22_STACK_SIZE, NULL, DHT22_PRIORITY, &task_dht22, DHT22_CORE_ID);

			g_dht22_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, DHT22 recollecter or sensor manager hasn't been registered");
		}
	}
}

void dht22_destroy(void){

	if(pthread_mutex_destroy(&mutex_dht22) != 0){
		ESP_LOGE(TAG,"Failed to destroy the DHT22 mutex");
	}
	else{
		ESP_LOGI(TAG, "DHT22 recollecter and sensor manager successfully destroyed");

		vTaskDelete(task_dht22);

		g_dht22_initialized = false;
	}
}

int dht22_add_sensor(int* gpios, union sensor_value_u* parameters){

	int res;

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		return -1;
	}

	res = gpios_manager_lock(gpios, DHT22_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_dht22);

		ESP_LOGI(TAG, "Sensor installed");

		dht22_cont++;

		dht22_gpios_array = (dht22_gpios_t*) realloc(dht22_gpios_array, sizeof(dht22_gpios_t) * dht22_cont);
		dht22_data_array = (dht22_data_t*) realloc(dht22_data_array, sizeof(dht22_data_t) * dht22_cont);

		humidity_alert_counter = (int*) realloc(humidity_alert_counter, sizeof(int) * dht22_cont);
		humidity_is_alerted = (bool*) realloc(humidity_is_alerted, sizeof(bool) * dht22_cont);

		temperature_alert_counter = (int*) realloc(temperature_alert_counter, sizeof(int) * dht22_cont);
		temperature_is_alerted = (bool*) realloc(temperature_is_alerted, sizeof(bool) * dht22_cont);

		dht22_gpios_array[dht22_cont - 1].data = gpios[0];

		dht22_data_array[dht22_cont - 1].humidity = 0.0;
		dht22_data_array[dht22_cont - 1].temperature = 0.0;

		humidity_alert_counter[dht22_cont - 1] = 0;
		humidity_is_alerted[dht22_cont - 1] = false;

		temperature_alert_counter[dht22_cont - 1] = 0;
		temperature_is_alerted[dht22_cont - 1] = false;

		nvs_app_set_uint8_value(nvs_DHT22_CONT_key,(uint8_t)dht22_cont);
		nvs_app_set_blob_value(nvs_DHT22_GPIOS_key,dht22_gpios_array,sizeof(dht22_gpios_t)*dht22_cont);

		pthread_mutex_unlock(&mutex_dht22);
	}

	return res;
}

int dht22_delete_sensor(int pos){

	int res;

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= dht22_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	res = gpios_manager_free(&dht22_gpios_array[pos].data, DHT22_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_dht22);

		ESP_LOGI(TAG, "Sensor deleted");

		shift_delete(pos);

		dht22_gpios_array = (dht22_gpios_t*) realloc(dht22_gpios_array, sizeof(dht22_gpios_t) * dht22_cont);
		dht22_data_array = (dht22_data_t*) realloc(dht22_data_array, sizeof(dht22_data_t) * dht22_cont);

		humidity_alert_counter = (int*) realloc(humidity_alert_counter, sizeof(int) * dht22_cont);
		humidity_is_alerted = (bool*) realloc(humidity_is_alerted, sizeof(bool) * dht22_cont);

		temperature_alert_counter = (int*) realloc(temperature_alert_counter, sizeof(int) * dht22_cont);
		temperature_is_alerted = (bool*) realloc(temperature_is_alerted, sizeof(bool) * dht22_cont);

		nvs_app_set_uint8_value(nvs_DHT22_CONT_key,(uint8_t)dht22_cont);
		nvs_app_set_blob_value(nvs_DHT22_GPIOS_key,dht22_gpios_array,sizeof(dht22_gpios_t)*dht22_cont);

		pthread_mutex_unlock(&mutex_dht22);
	}

	return res;
}

int dht22_set_gpios(int pos, int* gpios){

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= dht22_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	if(!gpios_manager_is_free(gpios[0])){
		ESP_LOGE(TAG, "Error, the GPIO selected is not available");
		return -1;
	}

	pthread_mutex_lock(&mutex_dht22);

	gpios_manager_lock(gpios, DHT22_N_GPIOS);

	gpios_manager_free(&dht22_gpios_array[pos].data, DHT22_N_GPIOS);

	dht22_gpios_array[pos].data = gpios[0];

	nvs_app_set_blob_value(nvs_DHT22_GPIOS_key,dht22_gpios_array,sizeof(dht22_gpios_t)*dht22_cont);

	pthread_mutex_unlock(&mutex_dht22);

	return 1;
}

int dht22_set_parameters(int pos, union sensor_value_u* parameters){
	return -1;
}

int dht22_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold){

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		return -1;
	}

	if(value < 0 || value > 1){
		ESP_LOGE(TAG, "Error, the value doesn't exist");
		return -1;
	}

	if(n_ticks <= 0){
		ESP_LOGE(TAG, "Error, the number of ticks can't be 0 or less than 0");
		return -1;
	}

	if(upper_threshold.fval <= lower_threshold.fval){
		ESP_LOGE(TAG, "Error, the thresholds don't make sense, upper is smaller than lower");
		return -1;
	}

	pthread_mutex_lock(&mutex_dht22);

	if(value == 0){ // Humidity
		dht22_alert_humidity = alert;
		dht22_humidity_ticks_to_alert = n_ticks;
		dht22_humidity_upper_threshold = upper_threshold.fval;
		dht22_humidity_lower_threshold = lower_threshold.fval;
	}
	else{ // Temperature
		dht22_alert_temperature = alert;
		dht22_temperature_ticks_to_alert = n_ticks;
		dht22_temperature_upper_threshold = upper_threshold.fval;
		dht22_temperature_lower_threshold = lower_threshold.fval;
	}

	pthread_mutex_unlock(&mutex_dht22);

	return 1;
}

sensor_data_t* dht22_get_sensors_data(int* number_of_sensors){

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		return NULL;
	}

	if(dht22_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = dht22_cont;

	sensor_data_t* aux = (sensor_data_t*) malloc(sizeof(sensor_data_t) * dht22_cont);
	sensor_value_t *aux2;

	pthread_mutex_lock(&mutex_dht22);

	for(int i = 0; i < dht22_cont; i++){

		aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * DHT22_N_VALUES);

		strcpy(aux[i].sensorName, "DHT22");
		aux[i].valuesLen = DHT22_N_VALUES;
		aux[i].sensor_values = aux2;

		aux[i].sensor_values[0].showOnLCD = DHT22_SHOW_HUMIDITY_ON_LCD;
		strcpy(aux[i].sensor_values[0].valueName,"Humidity");
		aux[i].sensor_values[0].sensor_value_type = FLOAT;
		aux[i].sensor_values[0].sensor_value.fval = dht22_data_array[i].humidity;
		aux[i].sensor_values[0].alert = dht22_alert_humidity;
		aux[i].sensor_values[0].ticks_to_alert = dht22_humidity_ticks_to_alert;
		aux[i].sensor_values[0].upper_threshold.fval = dht22_humidity_upper_threshold;
		aux[i].sensor_values[0].lower_threshold.fval = dht22_humidity_lower_threshold;

		aux[i].sensor_values[1].showOnLCD = DHT22_SHOW_TEMPERATURE_ON_LCD;
		strcpy(aux[i].sensor_values[1].valueName,"Temperature");
		aux[i].sensor_values[1].sensor_value_type = FLOAT;
		aux[i].sensor_values[1].sensor_value.fval = dht22_data_array[i].temperature;
		aux[i].sensor_values[1].alert = dht22_alert_temperature;
		aux[i].sensor_values[1].ticks_to_alert = dht22_temperature_ticks_to_alert;
		aux[i].sensor_values[1].upper_threshold.fval = dht22_temperature_upper_threshold;
		aux[i].sensor_values[1].lower_threshold.fval = dht22_temperature_lower_threshold;
	}

	pthread_mutex_unlock(&mutex_dht22);

	return aux;
}

sensor_gpios_info_t* dht22_get_sensors_gpios(int* number_of_sensors){

	if(!g_dht22_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the DHT22 without initializing it");
		return NULL;
	}

	if(dht22_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = dht22_cont;

	sensor_gpios_info_t* aux = (sensor_gpios_info_t*) malloc(sizeof(sensor_gpios_info_t) * dht22_cont);
	sensor_gpio_t *aux2;

	pthread_mutex_lock(&mutex_dht22);

	for(int i = 0; i < dht22_cont; i++){

		aux2 = (sensor_gpio_t *)malloc(sizeof(sensor_gpio_t) * DHT22_N_GPIOS);

		aux[i].gpiosLen = DHT22_N_GPIOS;
		aux[i].sensor_gpios = aux2;

		strcpy(aux[i].sensor_gpios[0].gpioName,"Data");
		aux[i].sensor_gpios[0].sensor_gpio = dht22_gpios_array[i].data;
	}

	pthread_mutex_unlock(&mutex_dht22);

	return aux;
}

sensor_additional_parameters_info_t* dht22_get_sensors_additional_parameters(int* number_of_sensors){
	return NULL;
}
