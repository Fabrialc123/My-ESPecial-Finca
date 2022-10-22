/*
 * lcd.c
 *
 *  Created on: 5 oct. 2022
 *      Author: Kike
 */

#include "gpios/lcd.h"

#include <stdbool.h>
#include <pthread.h>

#include "unistd.h"
#include "esp_err.h"
#include "esp_log.h"
#include "string.h"

static const char TAG[] = "lcd";

bool g_i2c_lcd_initialized 	= false;

static uint8_t g_display_mode 		= 0x00;
static uint8_t g_display_control 	= 0x00;
static uint8_t g_display_function 	= 0x00;

static pthread_mutex_t mutex_lcd;
static sensor_data_t *sensors;
static int sensors_n;

static int current_sensor;

/**
 * Initializes the I2C LCD controller
 */
static esp_err_t i2c_lcd_init(void){

	i2c_config_t conf = {
			.mode = I2C_MODE_MASTER,
			.sda_io_num = LCD_SDA_GPIO,
			.scl_io_num = LCD_SCL_GPIO,
			.sda_pullup_en = GPIO_PULLUP_ENABLE,
			.scl_pullup_en = GPIO_PULLUP_ENABLE,
			.master.clk_speed = LCD_DEFAULT_SPEED,
	};

	i2c_param_config(LCD_I2C_INSTANCE, &conf);

	return i2c_driver_install(LCD_I2C_INSTANCE, conf.mode, 0, 0, 0);
}

/**
 * Send a command to the I2C LCD controller
 */
static void lcd_send_command(char cmd){

	esp_err_t err;
	char cmd_upper4, cdm_lower4;
	uint8_t cmd_transmision[4];

	cmd_upper4 = (cmd & 0xf0);
	cdm_lower4 = ((cmd << 4) & 0xf0);

	cmd_transmision[0] = cmd_upper4|0x0C; // en=1, rs=0
	cmd_transmision[1] = cmd_upper4|0x08; // en=0, rs=0
	cmd_transmision[2] = cdm_lower4|0x0C; // en=1, rs=0
	cmd_transmision[3] = cdm_lower4|0x08; // en=0, rs=0

	err = i2c_master_write_to_device(LCD_I2C_INSTANCE, LCD_ADDRESS, cmd_transmision, sizeof(cmd_transmision), MAX_TICKS_TO_WAIT);
	if(err != 0) ESP_LOGE(TAG, "Error with the command sent");
}

/**
 * Send data to the I2C LCD controller
 */
static void lcd_send_data(char data){

	esp_err_t err;
	char data_upper4, data_lower4;
	uint8_t data_transmision[4];

	data_upper4 = (data & 0xf0);
	data_lower4 = ((data << 4) & 0xf0);

	data_transmision[0] = data_upper4|0x0D; // en=1, rs=1
	data_transmision[1] = data_upper4|0x09; // en=0, rs=1
	data_transmision[2] = data_lower4|0x0D; // en=1, rs=1
	data_transmision[3] = data_lower4|0x09; // en=0, rs=1

	err = i2c_master_write_to_device(LCD_I2C_INSTANCE, LCD_ADDRESS, data_transmision, sizeof(data_transmision), MAX_TICKS_TO_WAIT);
	if(err != 0) ESP_LOGE(TAG, "Error with the data sent");
}

void lcd_init(void){

	esp_err_t err;
	uint8_t cmd_tries[2] = {0x3C, 0x38};
	uint8_t cmd_4bit_interface[2] = {0x2C, 0x28};

	if(!g_i2c_lcd_initialized){

		if(i2c_lcd_init() == 0){

			if(pthread_mutex_init (&mutex_lcd, NULL) != 0){
				ESP_LOGE(TAG,"Failed to initialize the LCD mutex");
			}
			else{
				sensors = (sensor_data_t *)malloc(sizeof(sensor_data_t) * LCD_SENSOR_SIZE);
				sensors_n = 0;

				current_sensor = 0;

				g_i2c_lcd_initialized = true;
			}

		}
		else{
			ESP_LOGE(TAG, "Error with the I2C-LCD initialization");
		}

	}

	if(g_i2c_lcd_initialized){

		// 4 bit initialization
		usleep(50000);

		err = i2c_master_write_to_device(LCD_I2C_INSTANCE, LCD_ADDRESS, cmd_tries, sizeof(cmd_tries), MAX_TICKS_TO_WAIT);
		if(err != 0) ESP_LOGE(TAG, "Error with the first try of the initialization");

		usleep(4500);

		err = i2c_master_write_to_device(LCD_I2C_INSTANCE, LCD_ADDRESS, cmd_tries, sizeof(cmd_tries), MAX_TICKS_TO_WAIT);
		if(err != 0) ESP_LOGE(TAG, "Error with the second try of the initialization");

		usleep(4500);

		err = i2c_master_write_to_device(LCD_I2C_INSTANCE, LCD_ADDRESS, cmd_tries, sizeof(cmd_tries), MAX_TICKS_TO_WAIT);
		if(err != 0) ESP_LOGE(TAG, "Error with the third try of the initialization");

		usleep(150);

		err = i2c_master_write_to_device(LCD_I2C_INSTANCE, LCD_ADDRESS, cmd_4bit_interface, sizeof(cmd_4bit_interface), MAX_TICKS_TO_WAIT);
		if(err != 0) ESP_LOGE(TAG, "Error setting the 4 bit interface");

		//display initialization
		g_display_function = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
		lcd_send_command(LCD_FUNCTIONSET | g_display_function);

		g_display_control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);

		lcd_clear();

		g_display_mode = LCD_ENTRYLEFT | LCD_AUTOSCROLLOFF;
		lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
	}
}

void lcd_clear(void){

	if(g_i2c_lcd_initialized){
		lcd_send_command(LCD_CLEARDISPLAY);
		usleep(2000);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_home(void){

	if(g_i2c_lcd_initialized){
		lcd_send_command(LCD_RETURNHOME);
		usleep(2000);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_display_on(void){

	if(g_i2c_lcd_initialized){
		g_display_control |= LCD_DISPLAYON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_display_off(void){

	if(g_i2c_lcd_initialized){
		g_display_control &= ~LCD_DISPLAYON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_on(void){

	if(g_i2c_lcd_initialized){
		g_display_control |= LCD_CURSORON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_off(void){

	if(g_i2c_lcd_initialized){
		g_display_control &= ~LCD_CURSORON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_blink_on(void){

	if(g_i2c_lcd_initialized){
		g_display_control |= LCD_BLINKON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_blink_off(void){

	if(g_i2c_lcd_initialized){
		g_display_control &= ~LCD_BLINKON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_left_to_right(void){

	if(g_i2c_lcd_initialized){
		g_display_mode |= LCD_ENTRYLEFT;
		lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_right_to_left(void){

	if(g_i2c_lcd_initialized){
		g_display_mode &= ~LCD_ENTRYLEFT;
		lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_shift_display_left(void){

	if(g_i2c_lcd_initialized){
		lcd_send_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_shift_display_right(void){

	if(g_i2c_lcd_initialized){
		lcd_send_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_set_cursor(int row, int col){

	uint8_t pos;

	if(g_i2c_lcd_initialized){
		switch (row){

			case 0:
				pos = 0x80;
				break;

			case 1:
				pos = 0xC0;
				break;

			default:
				pos = 0x80;
				break;
		}

		lcd_send_command(pos | col);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_write(char *str){

	if(g_i2c_lcd_initialized){
		while(*str) lcd_send_data(*str++);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_one_more_sensor(void){

	if(g_i2c_lcd_initialized){
		pthread_mutex_lock(&mutex_lcd);

			if(sensors_n >= LCD_SENSOR_SIZE){
				ESP_LOGE(TAG, "ERROR, you can't manage more than %d sensors with the LCD", LCD_SENSOR_SIZE);
			}
			else{
				sensors_n++;
			}

		pthread_mutex_unlock(&mutex_lcd);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_one_less_sensor(int sensor_id){

	if(g_i2c_lcd_initialized){
		pthread_mutex_lock(&mutex_lcd);

		if((sensor_id >= sensors_n) || (sensor_id < 0)){
			ESP_LOGE(TAG, "ERROR, you can't delete a non-existant sensor");
		}
		else{
			for(int i = sensor_id; i < sensors_n - 1; i++){
				sensors[i] = sensors[i + 1];
			}

			memset(&sensors[sensors_n - 1], 0, sizeof(sensor_data_t));
			sensors_n--;
		}

		pthread_mutex_unlock(&mutex_lcd);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_update_sensor_data(int sensor_id, sensor_data_t sensor_data){

	if(g_i2c_lcd_initialized){
		pthread_mutex_lock(&mutex_lcd);

		if((sensor_id >= sensors_n) || (sensor_id < 0)){
			ESP_LOGE(TAG, "ERROR, you can't update a non-existant sensor");
		}
		else{
			sensors[sensor_id] = sensor_data;
		}

		pthread_mutex_unlock(&mutex_lcd);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_show_next_sensor_data(void){

	sensor_data_t sensor_data;

	if(g_i2c_lcd_initialized){

		pthread_mutex_lock(&mutex_lcd);

		sensor_data = sensors[current_sensor];

		pthread_mutex_unlock(&mutex_lcd);

		lcd_clear();

		// First row (Sensor ID + Sensor Name)

		char fr[MAX_CHARAC_ROW];
		char auxfr[10];

		memset(&fr, 0, MAX_CHARAC_ROW);
		memset(&auxfr, 0, 10);

		strcat(fr, "[Sensor id: ");

		sprintf(auxfr, "%d", current_sensor);

		strcat(fr, auxfr);
		strcat(fr, "] [Name: ");
		strncat(fr, sensor_data.sensorName, 15);
		strcat(fr, "]");

		lcd_set_cursor(0, 0);
		lcd_write(fr);

		// Second row (Sensor Values)

		char sr[MAX_CHARAC_ROW];
		char auxsr[10];

		memset(&sr, 0, MAX_CHARAC_ROW);
		memset(&auxsr, 0, 10);

		for(int i = 0; i < sensor_data.valuesLen; i++){

			strcat(sr, "[");

			if(sensor_data.sensor_values[i].sensor_value_type == INTEGER){
				strncat(sr, sensor_data.sensor_values[i].valueName, 5);
				strcat(sr, ": ");

				sprintf(auxsr, "%d", sensor_data.sensor_values[i].sensor_value.ival);

				strcat(sr, auxsr);
			}
			else if(sensor_data.sensor_values[i].sensor_value_type == FLOAT){
				strncat(sr, sensor_data.sensor_values[i].valueName, 5);
				strcat(sr, ": ");

				sprintf(auxsr, "%f", sensor_data.sensor_values[i].sensor_value.fval);

				strcat(sr, auxsr);
			}
			else{
				strncat(sr, sensor_data.sensor_values[i].valueName, 5);
				strcat(sr, ": ");
				strncat(sr, sensor_data.sensor_values[i].sensor_value.cval, 10);
			}

			if(i < sensor_data.valuesLen - 1){
				strcat(sr, "] ");
			}
			else{
				strcat(sr, "]");
			}
		}

		lcd_set_cursor(1, 0);
		lcd_write(sr);

		pthread_mutex_lock(&mutex_lcd);

		if(current_sensor < sensors_n - 1){
			current_sensor++;
		}
		else{
			current_sensor = 0;
		}

		pthread_mutex_unlock(&mutex_lcd);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}
