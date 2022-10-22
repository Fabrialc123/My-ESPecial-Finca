/*
 * lcd.h
 *
 *  Created on: 5 oct. 2022
 *      Author: Kike
 */

#ifndef MAIN_GPIOS_LCD_H_
#define MAIN_GPIOS_LCD_H_

#include "driver/i2c.h"
#include "recollecter.h"

#define LCD_SDA_GPIO			21
#define LCD_SCL_GPIO			22

#define LCD_DEFAULT_SPEED		100000 		// 100000 Hz -> 100 KHz

#define LCD_I2C_INSTANCE		I2C_NUM_0
#define LCD_ADDRESS				0x27		// Default address

#define MAX_TICKS_TO_WAIT		1000		// 1000 ms -> 1 s
#define MAX_CHARAC_ROW			40			// Only 16 are visible at once (per row)

#define LCD_SENSOR_SIZE 		5			// Should be the same number as RECOLLECTER_SIZE in "recollecter.h"

/**
 * [Note] 8 bits: bit 7 => D7 / bit 6 => D6 / bit 5 => D5 / bit 4 => D4 / bit 3 => LED+ / bit 2 => en / bit 1 => R/W / bit 0 => RS
 */

// Commands
#define LCD_CLEARDISPLAY 		0x01		// Clear the display
#define LCD_RETURNHOME 			0x02		// Move cursor to first row and first column (this command don't clean the display)
#define LCD_ENTRYMODESET 		0x04		// Change how the cursor works
#define LCD_DISPLAYCONTROL 		0x08		// Change how the display works
#define LCD_CURSORSHIFT 		0x10		// Force the movement of the cursor/display
#define LCD_FUNCTIONSET 		0x20		// Change how the LCD controller works

// Flags for display entry mode
#define LCD_ENTRYRIGHT 			0x00
#define LCD_ENTRYLEFT 			0x02
#define LCD_AUTOSCROLLON 		0x01
#define LCD_AUTOSCROLLOFF 		0x00

// Flags for display on/off control
#define LCD_DISPLAYON 			0x04
#define LCD_DISPLAYOFF 			0x00
#define LCD_CURSORON 			0x02
#define LCD_CURSOROFF 			0x00
#define LCD_BLINKON 			0x01
#define LCD_BLINKOFF 			0x00

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE 		0x08
#define LCD_CURSORMOVE 			0x00
#define LCD_MOVERIGHT 			0x04
#define LCD_MOVELEFT 			0x00

// Flags for function set
#define LCD_4BITMODE 			0x00
#define LCD_2LINE 				0x08
#define LCD_5x8DOTS 			0x00

/**
 * Initializes LCD controller
 */
void lcd_init(void);

/**
 * Clear the LCD entirely
 */
void lcd_clear(void);

/**
 * Set the cursor in the first row and first column (doesn't clear the display)
 */
void lcd_home(void);

/**
 * Turn on the display
 */
void lcd_display_on(void);

/**
 * Turn off the display (doesn't clear the DDRAM)
 */
void lcd_display_off(void);

/**
 * Turn on the visibility of the cursor's underline
 */
void lcd_cursor_on(void);

/**
 * Turn off the visibility of the cursor's underline
 */
void lcd_cursor_off(void);

/**
 * Turn on the blinking cursor
 */
void lcd_cursor_blink_on(void);

/**
 * Turn off the blinking cursor
 */
void lcd_cursor_blink_off(void);

/**
 * Change the text flow Left to Right
 */
void lcd_left_to_right(void);

/**
 * Change the text flow Right to Left
 */
void lcd_right_to_left(void);

/**
 * Shift the display to the Left
 */
void lcd_shift_display_left(void);

/**
 * Shift the display to the Right
 */
void lcd_shift_display_right(void);

/**
 * Set the cursor in the selected row and column
 * @param row, selected row (it has to be 0 <= x <= 1)
 * @param col, selected column (it has to be 0 <= y <= 15)
 * @note this function doesn't check if the parameters passed are right!
 */
void lcd_set_cursor(int row, int col);

/**
 * Write the text in the actual cursor position
 * @param *str, char pointer to the string message you want to show on the display
 */
void lcd_write(char *str);

/**
 * Indicates that one sensor is added to the system
 */
void lcd_one_more_sensor(void);

/**
 * Indicates that one sensor is removed from the system
 * @param sensor_id literally the ID of the sensor to be removed
 */
void lcd_one_less_sensor(int sensor_id);

/**
 * Updates the values of the given sensor
 * @param sensor_id literally the ID of the sensor to be updated
 * @param sensor_data the new values
 */
void lcd_update_sensor_data(int sensor_id, sensor_data_t sensor_data);

/**
 * Show on the LCD the data of the next sensor
 */
void lcd_show_next_sensor_data(void);

#endif /* MAIN_GPIOS_LCD_H_ */
