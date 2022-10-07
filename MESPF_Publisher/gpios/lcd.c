/*
 * lcd.c
 *
 *  Created on: 5 oct. 2022
 *      Author: Kike
 */

#include "gpios/lcd.h"

#include <stdbool.h>

#include "unistd.h"
#include "esp_err.h"
#include "esp_log.h"

static const char TAG[] = "lcd";

bool g_i2c_lcd_initialized 	= false;

static uint8_t g_display_mode 		= 0x00;
static uint8_t g_display_control 	= 0x00;
static uint8_t g_display_function 	= 0x00;


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
			g_i2c_lcd_initialized = true;
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

	lcd_send_command(LCD_CLEARDISPLAY);
	usleep(2000);
}

void lcd_home(void){

	lcd_send_command(LCD_RETURNHOME);
	usleep(2000);
}

void lcd_display_on(void){

	g_display_control |= LCD_DISPLAYON;
	lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
}

void lcd_display_off(void){

	g_display_control &= ~LCD_DISPLAYON;
	lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
}

void lcd_cursor_on(void){

	g_display_control |= LCD_CURSORON;
	lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
}

void lcd_cursor_off(void){

	g_display_control &= ~LCD_CURSORON;
	lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
}

void lcd_cursor_blink_on(void){

	g_display_control |= LCD_BLINKON;
	lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
}

void lcd_cursor_blink_off(void){

	g_display_control &= ~LCD_BLINKON;
	lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
}

void lcd_left_to_right(void){

	g_display_mode |= LCD_ENTRYLEFT;
	lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
}

void lcd_right_to_left(void){

	g_display_mode &= ~LCD_ENTRYLEFT;
	lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
}

void lcd_autoscroll_on(void){

	g_display_mode |= LCD_AUTOSCROLLON;
	lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
}

void lcd_autoscroll_off(void){

	g_display_mode &= ~LCD_AUTOSCROLLON;
	lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
}

void lcd_set_cursor(int row, int col){

	uint8_t pos;

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

void lcd_write(char *str){

	while(*str) lcd_send_data(*str++);
}

