/*
 * led_rgb.h
 *
 *  Created on: 13 jun. 2022
 *      Author: fabri
 */

#ifndef MAIN_GPIOS_LED_RGB_H_
#define MAIN_GPIOS_LED_RGB_H_

#define LED_RGB_RED_GPIO	15
#define LED_RGB_GREEN_GPIO	2
#define LED_RGB_BLUE_GPIO	4

#define LED_RGB_CHANNEL_NUM	3

typedef struct{
	int channel;
	int gpio;
	int mode;
	int timer_index;
}led_rgb_info_t;

led_rgb_info_t led_rgb_ch[LED_RGB_CHANNEL_NUM];



/**
 * Color to indicate Wifi app has started
 */
void led_rgb_wifi_app_started(void);

/**
 * Color to indicate HTTP server has started
 */
void led_rgb_http_server_started(void);

/**
 * Color to indicate that the ESP32 is connected to an Access Point (AP)
 */
void led_rgb_wifi_connected(void);

void led_rgb_test(void);

#endif /* MAIN_GPIOS_LED_RGB_H_ */
