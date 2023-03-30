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
#include <sys/time.h>
#include <esp_timer.h>
#include <nvs_app.h>

#include "unistd.h"
#include "esp_log.h"
#include "string.h"
#include "driver/gpio.h"
#include "mqtt/mqtt_commands.h"
#include "gpios_manager.h"
#include "sensors_manager.h"

// Values related to the HC_RS04 sensor in general

static TaskHandle_t task_hc_rs04 					= NULL;

static const char TAG[]  							= "hc_rs04";

bool g_hc_rs04_initialized 							= false;

static pthread_mutex_t mutex_hc_rs04;

// Arrays for the HC_RS04 sensors management

static hc_rs04_additional_params_t* hc_rs04_additional_params_array;
static hc_rs04_gpios_t* hc_rs04_gpios_array;
static hc_rs04_data_t* hc_rs04_data_array;

static int hc_rs04_cont								= 0;

// Values related to the alerts

static bool hc_rs04_alert_water_level				= false;
static int hc_rs04_water_level_ticks_to_alert		= 5;
static float hc_rs04_water_level_upper_threshold	= 1000.0;
static float hc_rs04_water_level_lower_threshold	= -1000.0;

static int* water_level_alert_counter;
static bool* water_level_is_alerted;

// NVS keys

#define nvs_HC_RS04_CONT_key		"hc_rs04_cont"
#define nvs_HC_RS04_GPIOS_key		"hc_rs04_gpios"
#define nvs_HC_RS04_PARAMETERS_key	"hc_rs04_param"


/**
 * Shift and delete
 */
static void shift_delete(int pos){

	for(int i = pos; i < hc_rs04_cont - 1; i++){

		hc_rs04_additional_params_array[pos].distance_between_sensor_and_tank = hc_rs04_additional_params_array[pos + 1].distance_between_sensor_and_tank;
		hc_rs04_additional_params_array[pos].tank_length = hc_rs04_additional_params_array[pos + 1].tank_length;

		hc_rs04_gpios_array[pos].echo = hc_rs04_gpios_array[pos + 1].echo;
		hc_rs04_gpios_array[pos].trig = hc_rs04_gpios_array[pos + 1].trig;

		hc_rs04_data_array[pos].water_level_percentage = hc_rs04_data_array[pos + 1].water_level_percentage;

		water_level_alert_counter[pos] = water_level_alert_counter[pos + 1];
		water_level_is_alerted[pos] = water_level_is_alerted[pos + 1];
	}

	hc_rs04_cont--;
}

/**
 * Task for HC_RS04
 */
static void hc_rs04_task(void *pvParameters){

	int64_t start, echo_start, time, timeout;
	bool fail;
	float distance, aux;

	int msg_id = 0;

	for(;;){

		pthread_mutex_lock(&mutex_hc_rs04);

		for(int i = 0; i < hc_rs04_cont; i++){

		    // Pull down trig pin for 4 us
		    gpio_set_level(hc_rs04_gpios_array[i].trig, 0);
		    usleep(4);

		    // Pull up trig pin for 10 us
		    gpio_set_level(hc_rs04_gpios_array[i].trig, 1);
		    usleep(10);

		    // Pull it down now
		    gpio_set_level(hc_rs04_gpios_array[i].trig, 0);

		    // Wait for echo
		    fail = false;
		    start = esp_timer_get_time();
		    while(!gpio_get_level(hc_rs04_gpios_array[i].echo) && !fail)
		        if(esp_timer_get_time() - start >= PING_TIMEOUT)
		        	fail = true;

		    if(fail)
		    	ESP_LOGE(TAG,"Error, echo HIGH signal not happened");
		    else{

				// Measuring
				fail = false;
				echo_start = esp_timer_get_time();
				time = echo_start;
				timeout = MAX_SENSE_DISTANCE * ROUNDTRIP;
				while (gpio_get_level(hc_rs04_gpios_array[i].echo) && !fail){
					time = esp_timer_get_time();
					if (esp_timer_get_time() - echo_start >= timeout)
						fail = true;
				}

				if(fail)
					ESP_LOGE(TAG,"Error, distance cannot be measured");
				else{
					distance = (float) (time - echo_start) / ROUNDTRIP;

					aux = 100 - (((distance - hc_rs04_additional_params_array[i].distance_between_sensor_and_tank) / hc_rs04_additional_params_array[i].tank_length) * 100);

					if(aux > 100.0){
						hc_rs04_data_array[i].water_level_percentage = 100.0;
					}
					else if(aux < 0.0){
						hc_rs04_data_array[i].water_level_percentage = 0.0;
					}
					else{
						hc_rs04_data_array[i].water_level_percentage = aux;
					}
				}
		    }

			// Water level alert
			if(hc_rs04_alert_water_level){

				// Update counter
				if((hc_rs04_data_array[i].water_level_percentage < hc_rs04_water_level_lower_threshold) && (water_level_alert_counter[i] > -hc_rs04_water_level_ticks_to_alert))
					water_level_alert_counter[i]--;
				else if((hc_rs04_data_array[i].water_level_percentage > hc_rs04_water_level_upper_threshold) && (water_level_alert_counter[i] < hc_rs04_water_level_ticks_to_alert))
					water_level_alert_counter[i]++;
				else{
					if(water_level_alert_counter[i] > 0)
						water_level_alert_counter[i]--;
					else if(water_level_alert_counter[i] < 0)
						water_level_alert_counter[i]++;
				}

				// Check if the value can be alerted again
				if((water_level_alert_counter[i] == 0) && water_level_is_alerted[i]){
					water_level_is_alerted[i] = false;
				}

				// Check if it is the moment to alert
				if((water_level_alert_counter[i] == -hc_rs04_water_level_ticks_to_alert) && !water_level_is_alerted[i]){

					mqtt_app_send_alert("HC_RS04", msg_id, "WARNING in Sensor HC_RS04! exceed on lower threshold (value: water_level_percentage)");

					water_level_is_alerted[i] = true;
					msg_id++;
				}
				else if((water_level_alert_counter[i] == hc_rs04_water_level_ticks_to_alert) && !water_level_is_alerted[i]){

					mqtt_app_send_alert("HC_RS04", msg_id, "WARNING in Sensor HC_RS04! exceed on upper threshold (value: water_level_percentage)");

					water_level_is_alerted[i] = true;
					msg_id++;
				}
			}
		}

		pthread_mutex_unlock(&mutex_hc_rs04);

		vTaskDelay(HC_RS04_TIME_TO_UPDATE_DATA);
	}
}

void hc_rs04_startup(void){
	size_t size = 0;
	uint8_t aux = 0;

	nvs_app_get_uint8_value(nvs_HC_RS04_CONT_key,&aux);

	hc_rs04_cont = aux;

	hc_rs04_additional_params_array = (hc_rs04_additional_params_t*) malloc(sizeof(hc_rs04_additional_params_t) * hc_rs04_cont);
	hc_rs04_gpios_array = (hc_rs04_gpios_t*) malloc(sizeof(hc_rs04_gpios_t) * hc_rs04_cont);
	hc_rs04_data_array = (hc_rs04_data_t*) malloc(sizeof(hc_rs04_data_t) * hc_rs04_cont);

	water_level_alert_counter = (int*) malloc(sizeof(int) * hc_rs04_cont);
	water_level_is_alerted = (bool*) malloc(sizeof(bool) * hc_rs04_cont);

	if(hc_rs04_cont > 0){
		nvs_app_get_blob_value(nvs_HC_RS04_GPIOS_key,NULL,&size);
		if(size != 0)
			nvs_app_get_blob_value(nvs_HC_RS04_GPIOS_key,hc_rs04_gpios_array,&size);

		int gpios[HC_RS04_N_GPIOS];
		for(int i = 0; i < hc_rs04_cont; i++){
			gpios[0] = hc_rs04_gpios_array[i].trig;
			gpios[1] = hc_rs04_gpios_array[i].echo;
			gpios_manager_lock(gpios, HC_RS04_N_GPIOS);

		    gpio_reset_pin(gpios[0]);
		    gpio_reset_pin(gpios[1]);
		    gpio_set_direction(gpios[0], GPIO_MODE_OUTPUT);
		    gpio_set_direction(gpios[1], GPIO_MODE_INPUT);
		    gpio_set_level(gpios[0], 0);
		}

		size = 0;

		nvs_app_get_blob_value(nvs_HC_RS04_PARAMETERS_key,NULL,&size);
		if(size != 0)
			nvs_app_get_blob_value(nvs_HC_RS04_PARAMETERS_key,hc_rs04_additional_params_array,&size);

		hc_rs04_init();
	}
}

void hc_rs04_init(void){

	if(pthread_mutex_init(&mutex_hc_rs04, NULL) != 0){
		ESP_LOGE(TAG,"Failed to initialize the HC_RS04 mutex");
	}
	else{
		int res_rec, res_sen;

		res_rec = register_recollecter(&hc_rs04_get_sensors_data, &hc_rs04_get_sensors_gpios, &hc_rs04_get_sensors_additional_parameters);
		res_sen = sensors_manager_add(&hc_rs04_destroy, &hc_rs04_add_sensor, &hc_rs04_delete_sensor, &hc_rs04_set_gpios, &hc_rs04_set_parameters, &hc_rs04_set_alert_values);

		if(res_rec == 1 && res_sen == 1){
			ESP_LOGI(TAG, "HC-RS04 recollecter successfully registered");

			xTaskCreatePinnedToCore(&hc_rs04_task, "hc_rs04_task", HC_RS04_STACK_SIZE, NULL, HC_RS04_PRIORITY, &task_hc_rs04, HC_RS04_CORE_ID);

			g_hc_rs04_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error, HC-RS04 recollecter or sensor manager hasn't been registered");
		}
	}
}

void hc_rs04_destroy(void){

	if(pthread_mutex_destroy(&mutex_hc_rs04) != 0){
		ESP_LOGE(TAG,"Failed to destroy the HC-RS04 mutex");
	}
	else{
		ESP_LOGI(TAG, "HC-RS04 recollecter and sensor manager successfully destroyed");

		vTaskDelete(task_hc_rs04);

		g_hc_rs04_initialized = false;
	}
}

int hc_rs04_add_sensor(int* gpios, union sensor_value_u* parameters){

	int res;

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return -1;
	}

	if(parameters[0].ival <= 0 || parameters[1].ival <= 0){
		ESP_LOGE(TAG, "Error, the distances can't be 0 or less than 0 centimeters");
		return -1;
	}

	res = gpios_manager_lock(gpios, HC_RS04_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_hc_rs04);

		ESP_LOGI(TAG, "Sensor installed");

	    gpio_reset_pin(gpios[0]);
	    gpio_reset_pin(gpios[1]);
	    gpio_set_direction(gpios[0], GPIO_MODE_OUTPUT);
	    gpio_set_direction(gpios[1], GPIO_MODE_INPUT);

	    gpio_set_level(gpios[0], 0);

		hc_rs04_cont++;

		hc_rs04_additional_params_array = (hc_rs04_additional_params_t*) realloc(hc_rs04_additional_params_array, sizeof(hc_rs04_additional_params_t) * hc_rs04_cont);
		hc_rs04_gpios_array = (hc_rs04_gpios_t*) realloc(hc_rs04_gpios_array, sizeof(hc_rs04_gpios_t) * hc_rs04_cont);
		hc_rs04_data_array = (hc_rs04_data_t*) realloc(hc_rs04_data_array, sizeof(hc_rs04_data_t) * hc_rs04_cont);

		water_level_alert_counter = (int*) realloc(water_level_alert_counter, sizeof(int) * hc_rs04_cont);
		water_level_is_alerted = (bool*) realloc(water_level_is_alerted, sizeof(bool) * hc_rs04_cont);

		hc_rs04_additional_params_array[hc_rs04_cont - 1].distance_between_sensor_and_tank = parameters[0].ival;
		hc_rs04_additional_params_array[hc_rs04_cont - 1].tank_length = parameters[1].ival;

		hc_rs04_gpios_array[hc_rs04_cont - 1].trig = gpios[0];
		hc_rs04_gpios_array[hc_rs04_cont - 1].echo = gpios[1];

		hc_rs04_data_array[hc_rs04_cont - 1].water_level_percentage = 0.0;

		water_level_alert_counter[hc_rs04_cont - 1] = 0;
		water_level_is_alerted[hc_rs04_cont - 1] = false;

		nvs_app_set_uint8_value(nvs_HC_RS04_CONT_key,(uint8_t)hc_rs04_cont);
		nvs_app_set_blob_value(nvs_HC_RS04_GPIOS_key,hc_rs04_gpios_array,sizeof(hc_rs04_gpios_t)*hc_rs04_cont);
		nvs_app_set_blob_value(nvs_HC_RS04_PARAMETERS_key,hc_rs04_additional_params_array,sizeof(hc_rs04_additional_params_t)*hc_rs04_cont);

		pthread_mutex_unlock(&mutex_hc_rs04);
	}

	return res;
}

int hc_rs04_delete_sensor(int pos){

	int res;

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= hc_rs04_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	int gpios[HC_RS04_N_GPIOS] = {hc_rs04_gpios_array[pos].trig, hc_rs04_gpios_array[pos].echo};

	res = gpios_manager_free(gpios, HC_RS04_N_GPIOS);

	if(res == 1){

		pthread_mutex_lock(&mutex_hc_rs04);

		ESP_LOGI(TAG, "Sensor deleted");

	    gpio_reset_pin(gpios[0]);
	    gpio_reset_pin(gpios[1]);

		shift_delete(pos);

		hc_rs04_additional_params_array = (hc_rs04_additional_params_t*) realloc(hc_rs04_additional_params_array, sizeof(hc_rs04_additional_params_t) * hc_rs04_cont);
		hc_rs04_gpios_array = (hc_rs04_gpios_t*) realloc(hc_rs04_gpios_array, sizeof(hc_rs04_gpios_t) * hc_rs04_cont);
		hc_rs04_data_array = (hc_rs04_data_t*) realloc(hc_rs04_data_array, sizeof(hc_rs04_data_t) * hc_rs04_cont);

		water_level_alert_counter = (int*) realloc(water_level_alert_counter, sizeof(int) * hc_rs04_cont);
		water_level_is_alerted = (bool*) realloc(water_level_is_alerted, sizeof(bool) * hc_rs04_cont);

		nvs_app_set_uint8_value(nvs_HC_RS04_CONT_key,(uint8_t)hc_rs04_cont);
		nvs_app_set_blob_value(nvs_HC_RS04_GPIOS_key,hc_rs04_gpios_array,sizeof(hc_rs04_gpios_t)*hc_rs04_cont);
		nvs_app_set_blob_value(nvs_HC_RS04_PARAMETERS_key,hc_rs04_additional_params_array,sizeof(hc_rs04_additional_params_t)*hc_rs04_cont);

		pthread_mutex_unlock(&mutex_hc_rs04);
	}

	return res;
}

int hc_rs04_set_gpios(int pos, int* gpios){

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= hc_rs04_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	if(!gpios_manager_is_free(gpios[0]) || !gpios_manager_is_free(gpios[1])){
		ESP_LOGE(TAG, "Error, the GPIOS selected are not available");
		return -1;
	}

	pthread_mutex_lock(&mutex_hc_rs04);

	int gpiosIns[HC_RS04_N_GPIOS] = {hc_rs04_gpios_array[pos].trig, hc_rs04_gpios_array[pos].echo};

	gpios_manager_lock(gpios, HC_RS04_N_GPIOS);

	gpios_manager_free(gpiosIns, HC_RS04_N_GPIOS);

    gpio_reset_pin(gpiosIns[0]);
    gpio_reset_pin(gpiosIns[1]);

    gpio_reset_pin(gpios[0]);
    gpio_reset_pin(gpios[1]);
    gpio_set_direction(gpios[0], GPIO_MODE_OUTPUT);
    gpio_set_direction(gpios[1], GPIO_MODE_INPUT);

    gpio_set_level(gpios[0], 0);

	hc_rs04_gpios_array[pos].trig = gpios[0];
	hc_rs04_gpios_array[pos].echo = gpios[1];

	nvs_app_set_blob_value(nvs_HC_RS04_GPIOS_key,hc_rs04_gpios_array,sizeof(hc_rs04_gpios_t)*hc_rs04_cont);

	pthread_mutex_unlock(&mutex_hc_rs04);

	return 1;
}

int hc_rs04_set_parameters(int pos, union sensor_value_u* parameters){

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return -1;
	}

	if(pos < 0 || pos >= hc_rs04_cont){
		ESP_LOGE(TAG, "Error, the position is invalid");
		return -1;
	}

	if(parameters[0].ival <= 0 || parameters[1].ival <= 0){
		ESP_LOGE(TAG, "Error, the distances can't be 0 or less than 0 centimeters");
		return -1;
	}

	pthread_mutex_lock(&mutex_hc_rs04);

	hc_rs04_additional_params_array[pos].distance_between_sensor_and_tank = parameters[0].ival;
	hc_rs04_additional_params_array[pos].tank_length = parameters[1].ival;

	nvs_app_set_blob_value(nvs_HC_RS04_PARAMETERS_key,hc_rs04_additional_params_array,sizeof(hc_rs04_additional_params_t)*hc_rs04_cont);

	pthread_mutex_unlock(&mutex_hc_rs04);

	return 1;
}

int hc_rs04_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold){

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return -1;
	}

	if(value < 0 || value > 0){
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

	pthread_mutex_lock(&mutex_hc_rs04);

	hc_rs04_alert_water_level = alert;
	hc_rs04_water_level_ticks_to_alert = n_ticks;
	hc_rs04_water_level_upper_threshold = upper_threshold.fval;
	hc_rs04_water_level_lower_threshold = lower_threshold.fval;

	pthread_mutex_unlock(&mutex_hc_rs04);

	return 1;
}

sensor_data_t* hc_rs04_get_sensors_data(int* number_of_sensors){

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return NULL;
	}

	if(hc_rs04_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = hc_rs04_cont;

	sensor_data_t* aux = (sensor_data_t*) malloc(sizeof(sensor_data_t) * hc_rs04_cont);
	sensor_value_t *aux2;

	pthread_mutex_lock(&mutex_hc_rs04);

	for(int i = 0; i < hc_rs04_cont; i++){

		aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * HC_RS04_N_VALUES);

		strcpy(aux[i].sensorName, "HC-RS04");
		aux[i].valuesLen = HC_RS04_N_VALUES;
		aux[i].sensor_values = aux2;

		aux[i].sensor_values[0].showOnLCD = HC_RS04_SHOW_WATER_LEVEL_ON_LCD;
		strcpy(aux[i].sensor_values[0].valueName,"Water level");
		aux[i].sensor_values[0].sensor_value_type = FLOAT;
		aux[i].sensor_values[0].sensor_value.fval = hc_rs04_data_array[i].water_level_percentage;
		aux[i].sensor_values[0].alert = hc_rs04_alert_water_level;
		aux[i].sensor_values[0].ticks_to_alert = hc_rs04_water_level_ticks_to_alert;
		aux[i].sensor_values[0].upper_threshold.fval = hc_rs04_water_level_upper_threshold;
		aux[i].sensor_values[0].lower_threshold.fval = hc_rs04_water_level_lower_threshold;
	}

	pthread_mutex_unlock(&mutex_hc_rs04);

	return aux;
}

sensor_gpios_info_t* hc_rs04_get_sensors_gpios(int* number_of_sensors){

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return NULL;
	}

	if(hc_rs04_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = hc_rs04_cont;

	sensor_gpios_info_t* aux = (sensor_gpios_info_t*) malloc(sizeof(sensor_gpios_info_t) * hc_rs04_cont);
	sensor_gpio_t *aux2;

	pthread_mutex_lock(&mutex_hc_rs04);

	for(int i = 0; i < hc_rs04_cont; i++){

		aux2 = (sensor_gpio_t *)malloc(sizeof(sensor_gpio_t) * HC_RS04_N_GPIOS);

		aux[i].gpiosLen = HC_RS04_N_GPIOS;
		aux[i].sensor_gpios = aux2;

		strcpy(aux[i].sensor_gpios[0].gpioName,"Trig");
		aux[i].sensor_gpios[0].sensor_gpio = hc_rs04_gpios_array[i].trig;

		strcpy(aux[i].sensor_gpios[1].gpioName,"Echo");
		aux[i].sensor_gpios[1].sensor_gpio = hc_rs04_gpios_array[i].echo;
	}

	pthread_mutex_unlock(&mutex_hc_rs04);

	return aux;
}

sensor_additional_parameters_info_t* hc_rs04_get_sensors_additional_parameters(int* number_of_sensors){

	if(!g_hc_rs04_initialized){
		ESP_LOGE(TAG, "Error, you can't operate with the HC-RS04 without initializing it");
		return NULL;
	}

	if(hc_rs04_cont == 0){
		ESP_LOGI(TAG, "There is no sensors of this type");
		return NULL;
	}

	*number_of_sensors = hc_rs04_cont;

	sensor_additional_parameters_info_t* aux = (sensor_additional_parameters_info_t*) malloc(sizeof(sensor_additional_parameters_info_t) * hc_rs04_cont);
	sensor_additional_parameter_t *aux2;

	pthread_mutex_lock(&mutex_hc_rs04);

	for(int i = 0; i < hc_rs04_cont; i++){

		aux2 = (sensor_additional_parameter_t *)malloc(sizeof(sensor_additional_parameter_t) * HC_RS04_N_ADDITIONAL_PARAMS);

		aux[i].parametersLen = HC_RS04_N_ADDITIONAL_PARAMS;
		aux[i].sensor_parameters = aux2;

		strcpy(aux[i].sensor_parameters[0].parameterName,"Sensor - Tank (cm)");
		aux[i].sensor_parameters[0].sensor_parameter_type = INTEGER;
		aux[i].sensor_parameters[0].sensor_parameter.ival = hc_rs04_additional_params_array[i].distance_between_sensor_and_tank;

		strcpy(aux[i].sensor_parameters[1].parameterName,"Tank depth (cm)");
		aux[i].sensor_parameters[1].sensor_parameter_type = INTEGER;
		aux[i].sensor_parameters[1].sensor_parameter.ival = hc_rs04_additional_params_array[i].tank_length;
	}

	pthread_mutex_unlock(&mutex_hc_rs04);

	return aux;
}
