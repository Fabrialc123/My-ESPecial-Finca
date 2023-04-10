/*
 * lcd.c
 *
 *  Created on: 5 oct. 2022
 *      Author: Kike
 */

#include "gpios/lcd.h"
#include "recollecter.h"
#include "task_common.h"

#include <stdbool.h>

#include "unistd.h"
#include "esp_err.h"
#include "esp_log.h"
#include "string.h"

static const char TAG[] = "lcd";

bool g_i2c_initialized 	= false;
bool g_lcd_initialized	= false;

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

/**
 * Task for LCD
 */

static void lcd_task(void *pvParameters){

	int fr_size, sr_size, move_n, n_units, n_free;
	sensor_data_t* aux;
	char number[12];

	for(;;){

		for(int i = 0; i < get_recollecters_size(); i++){

			aux = get_sensor_data(i, &n_units);

			if(aux != NULL){

				lcd_clear();

				// First row (Sensor name)

				char sn[MAX_CHARAC_ROW];
				memset(&sn, 0, MAX_CHARAC_ROW);

				strcat(sn, "[Name: ");
				strncat(sn, aux[0].sensorName, 32);
				strcat(sn, "]");

				lcd_write(sn);

				// Second row (Number of units)

				lcd_set_cursor(1, 0);

				char nu[MAX_CHARAC_ROW];
				memset(&nu, 0, MAX_CHARAC_ROW);

				strcat(nu, "[Units: ");
				snprintf(number, 12, "%d", n_units);
				strcat(nu, number);
				strcat(nu, "]");

				lcd_write(nu);

				vTaskDelay(LCD_TIME_BEFORE_UNITS);

				for(int j = 0; j < n_units; j++){

					if(n_units > 1){
						lcd_clear();

						// First row (Sensor unit)

						char su[MAX_CHARAC_ROW];
						memset(&su, 0, MAX_CHARAC_ROW);

						strcat(su, "    [Unit ");
						snprintf(number, 12, "%d", j);
						strcat(su, number);
						strcat(su, "]");

						lcd_write(su);

						vTaskDelay(LCD_TIME_BEFORE_VALUES);
					}

					lcd_clear();

					// First row (First value + Third value) -> Each one will take 20 spaces at max

					char fv[MAX_CHARAC_ROW/2];
					char tv[MAX_CHARAC_ROW/2];
					memset(&fv, 0, MAX_CHARAC_ROW/2);
					memset(&tv, 0, MAX_CHARAC_ROW/2);

					// First value
					if(aux[j].sensor_values[0].showOnLCD == true){

						strcat(fv, "[");									// 1 character
						strncat(fv, aux[j].sensor_values[0].valueName, 4);	// 4 characters at max
						strcat(fv, ": ");									// 2 characters

						// 12 characters by all means
						if(aux[j].sensor_values[0].sensor_value_type == INTEGER){
							snprintf(number, 12, "%d", aux[j].sensor_values[0].sensor_value.ival);
							strcat(fv, number);
						}
						else if(aux[j].sensor_values[0].sensor_value_type == FLOAT){
							snprintf(number, 12, "%f", aux[j].sensor_values[0].sensor_value.fval);
							strcat(fv, number);
						}
						else{
							strncat(fv, aux[j].sensor_values[0].sensor_value.cval, 12);
						}

						strcat(fv, "]");									// 1 character

						lcd_write(fv);
					}

					// Third value
					if(aux[j].valuesLen >= 3 && aux[j].sensor_values[2].showOnLCD == true){
						lcd_set_cursor(0, 20);

						strcat(tv, "[");									// 1 character
						strncat(tv, aux[j].sensor_values[2].valueName, 4);	// 4 characters at max
						strcat(tv, ": ");									// 2 characters

						// 12 characters by all means
						if(aux[j].sensor_values[2].sensor_value_type == INTEGER){
							snprintf(number, 12, "%d", aux[j].sensor_values[2].sensor_value.ival);
							strcat(tv, number);
						}
						else if(aux[j].sensor_values[2].sensor_value_type == FLOAT){
							snprintf(number, 12, "%f", aux[j].sensor_values[2].sensor_value.fval);
							strcat(tv, number);
						}
						else{
							strncat(tv, aux[j].sensor_values[2].sensor_value.cval, 12);
						}

						strcat(tv, "]");									// 1 character

						lcd_write(tv);
					}

					// Second row (Second value + Fourth value) -> Each one will take 20 spaces at max

					char sv[MAX_CHARAC_ROW/2];
					char fov[MAX_CHARAC_ROW/2];
					memset(&sv, 0, MAX_CHARAC_ROW/2);
					memset(&fov, 0, MAX_CHARAC_ROW/2);

					// Second value
					if(aux[j].valuesLen >= 2 && aux[j].sensor_values[1].showOnLCD == true){
						lcd_set_cursor(1, 0);

						strcat(sv, "[");									// 1 character
						strncat(sv, aux[j].sensor_values[1].valueName, 4);	// 4 characters at max
						strcat(sv, ": ");									// 2 characters

						// 12 characters by all means
						if(aux[j].sensor_values[1].sensor_value_type == INTEGER){
							snprintf(number, 12, "%d", aux[j].sensor_values[1].sensor_value.ival);
							strcat(sv, number);
						}
						else if(aux[j].sensor_values[1].sensor_value_type == FLOAT){
							snprintf(number, 12, "%f", aux[j].sensor_values[1].sensor_value.fval);
							strcat(sv, number);
						}
						else{
							strncat(sv, aux[j].sensor_values[1].sensor_value.cval, 12);
						}

						strcat(sv, "]");									// 1 character

						lcd_write(sv);
					}

					// Fourth value
					if(aux[j].valuesLen >= 4 && aux[j].sensor_values[3].showOnLCD == true){
						lcd_set_cursor(1, 20);

						strcat(fov, "[");									// 1 character
						strncat(fov, aux[j].sensor_values[3].valueName, 4);	// 4 characters at max
						strcat(fov, ": ");									// 2 characters

						// 12 characters by all means
						if(aux[j].sensor_values[3].sensor_value_type == INTEGER){
							snprintf(number, 12, "%d", aux[j].sensor_values[3].sensor_value.ival);
							strcat(fov, number);
						}
						else if(aux[j].sensor_values[3].sensor_value_type == FLOAT){
							snprintf(number, 12, "%f", aux[j].sensor_values[3].sensor_value.fval);
							strcat(fov, number);
						}
						else{
							strncat(fov, aux[j].sensor_values[3].sensor_value.cval, 12);
						}

						strcat(fov, "]");									// 1 character

						lcd_write(fov);
					}

					// Display movement
					if(aux[j].valuesLen >= 4){
						if(aux[j].sensor_values[2].showOnLCD == true){fr_size = 20 + strlen(tv);}
						else if(aux[j].sensor_values[0].showOnLCD == true){fr_size = strlen(fv);}
						else{fr_size = 0;}

						if(aux[j].sensor_values[3].showOnLCD == true){sr_size = 20 + strlen(fov);}
						else if(aux[j].sensor_values[1].showOnLCD == true){sr_size = strlen(sv);}
						else{sr_size = 0;}
					}
					else if(aux[j].valuesLen == 3){
						if(aux[j].sensor_values[2].showOnLCD == true){fr_size = 20 + strlen(tv);}
						else if(aux[j].sensor_values[0].showOnLCD == true){fr_size = strlen(fv);}
						else{fr_size = 0;}

						if(aux[j].sensor_values[1].showOnLCD == true){sr_size = strlen(sv);}
						else{sr_size = 0;}
					}
					else if(aux[j].valuesLen == 2){
						if(aux[j].sensor_values[0].showOnLCD == true){fr_size = strlen(fv);}
						else{fr_size = 0;}

						if(aux[j].sensor_values[1].showOnLCD == true){sr_size = strlen(sv);}
						else{sr_size = 0;}
					}
					else{
						if(aux[j].sensor_values[0].showOnLCD == true){fr_size = strlen(fv);}
						else{fr_size = 0;}

						sr_size = 0;
					}

					if(fr_size > sr_size){
						move_n = fr_size - 16;
					}
					else{
						move_n = sr_size - 16;
					}

					if(move_n > 0){
						vTaskDelay(LCD_TIME_BEFORE_MOVE);

						for(int i = 0; i < move_n; i++){
							lcd_shift_display_left();
							vTaskDelay(LCD_TIME_MOVE_FREC);
						}
					}

					vTaskDelay(LCD_TIME_TO_SHOW_NEXT);
				}

				// Free sensor data pointers
				n_free = 0;

				do{
					free(aux[n_free].sensor_values);
					n_free++;
				}while (n_free < n_units);
				free(aux);
			}
			else{
				ESP_LOGE(TAG, "Error take place with the sensor that has ID %d", i);

				vTaskDelay(LCD_TIME_WHEN_FAIL);
			}
		}
		if(get_recollecters_size() == 0)
			vTaskDelay(LCD_TIME_WHEN_NOTHING);
	}
}

void lcd_init(void){

	esp_err_t err;
	uint8_t cmd_tries[2] = {0x3C, 0x38};
	uint8_t cmd_4bit_interface[2] = {0x2C, 0x28};

	if(!g_i2c_initialized){

		if(i2c_lcd_init() == 0){
			g_i2c_initialized = true;
		}
		else{
			ESP_LOGE(TAG, "Error with the I2C-LCD initialization");
		}
	}

	if(g_i2c_initialized){

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

		g_lcd_initialized = true;

		lcd_clear();

		g_display_mode = LCD_ENTRYLEFT | LCD_AUTOSCROLLOFF;
		lcd_send_command(LCD_ENTRYMODESET | g_display_mode);

		xTaskCreatePinnedToCore(&lcd_task, "lcd_task", LCD_STACK_SIZE, NULL, LCD_PRIORITY, NULL, LCD_CORE_ID);
	}
}

void lcd_clear(void){

	if(g_lcd_initialized){
		lcd_send_command(LCD_CLEARDISPLAY);
		usleep(2000);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_home(void){

	if(g_lcd_initialized){
		lcd_send_command(LCD_RETURNHOME);
		usleep(2000);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_display_on(void){

	if(g_lcd_initialized){
		g_display_control |= LCD_DISPLAYON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_display_off(void){

	if(g_lcd_initialized){
		g_display_control &= ~LCD_DISPLAYON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_on(void){

	if(g_lcd_initialized){
		g_display_control |= LCD_CURSORON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_off(void){

	if(g_lcd_initialized){
		g_display_control &= ~LCD_CURSORON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_blink_on(void){

	if(g_lcd_initialized){
		g_display_control |= LCD_BLINKON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_cursor_blink_off(void){

	if(g_lcd_initialized){
		g_display_control &= ~LCD_BLINKON;
		lcd_send_command(LCD_DISPLAYCONTROL | g_display_control);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_left_to_right(void){

	if(g_lcd_initialized){
		g_display_mode |= LCD_ENTRYLEFT;
		lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_right_to_left(void){

	if(g_lcd_initialized){
		g_display_mode &= ~LCD_ENTRYLEFT;
		lcd_send_command(LCD_ENTRYMODESET | g_display_mode);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_shift_display_left(void){

	if(g_lcd_initialized){
		lcd_send_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_shift_display_right(void){

	if(g_lcd_initialized){
		lcd_send_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}

void lcd_set_cursor(int row, int col){

	uint8_t pos;

	if(g_lcd_initialized){
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

	if(g_lcd_initialized){
		while(*str) lcd_send_data(*str++);
	}
	else{
		ESP_LOGE(TAG, "Error, you can't operate with the LCD without initializing it");
	}
}
