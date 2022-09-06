/*
 * led_rgb.c
 *
 *  Created on: 13 jun. 2022
 *      Author: fabri
 */
#include "gpios/led_rgb.h"

#include <stdbool.h>

#include "driver/ledc.h"

bool g_pwn_init_handle = false;

/**
 * Initializes the LED RGB settings per channel, including
 * the GPIO for each color, mode and timer configuration.
 */
static void led_rgb_pwn_init(void){
	int rgb_ch;

	led_rgb_ch[0].channel =		LEDC_CHANNEL_0;
	led_rgb_ch[0].gpio =		LED_RGB_RED_GPIO;
	led_rgb_ch[0].mode =		LEDC_HIGH_SPEED_MODE;
	led_rgb_ch[0].timer_index =	LEDC_TIMER_0;

	led_rgb_ch[1].channel =		LEDC_CHANNEL_1;
	led_rgb_ch[1].gpio =		LED_RGB_GREEN_GPIO;
	led_rgb_ch[1].mode =		LEDC_HIGH_SPEED_MODE;
	led_rgb_ch[1].timer_index =	LEDC_TIMER_0;

	led_rgb_ch[2].channel =		LEDC_CHANNEL_2;
	led_rgb_ch[2].gpio =		LED_RGB_BLUE_GPIO;
	led_rgb_ch[2].mode =		LEDC_HIGH_SPEED_MODE;
	led_rgb_ch[2].timer_index =	LEDC_TIMER_0;

	ledc_timer_config_t ledc_timer =
	{
			.duty_resolution = LEDC_TIMER_8_BIT,
			.freq_hz =			100,
			.speed_mode =		LEDC_HIGH_SPEED_MODE,
			.timer_num =		LEDC_TIMER_0
	};
	ledc_timer_config(&ledc_timer);

	for(rgb_ch = 0; rgb_ch < LED_RGB_CHANNEL_NUM; rgb_ch++){
		ledc_channel_config_t ledc_configuration =
		{
				.channel =		led_rgb_ch[rgb_ch].channel,
				.duty =			0,
				.hpoint =		0,
				.gpio_num = 	led_rgb_ch[rgb_ch].gpio,
				.intr_type =	LEDC_INTR_DISABLE,
				.speed_mode =	led_rgb_ch[rgb_ch].mode,
				.timer_sel =	led_rgb_ch[rgb_ch].timer_index
		};
		ledc_channel_config(&ledc_configuration);
	}

	g_pwn_init_handle = true;
}

/**
 * Sets the RGB color
 */
static void led_rgb_set_color(uint8_t red, uint8_t green, uint8_t blue){
	ledc_set_duty(led_rgb_ch[0].mode, led_rgb_ch[0].channel, red);
	ledc_update_duty(led_rgb_ch[0].mode, led_rgb_ch[0].channel);

	ledc_set_duty(led_rgb_ch[1].mode, led_rgb_ch[1].channel, green);
	ledc_update_duty(led_rgb_ch[1].mode, led_rgb_ch[1].channel);

	ledc_set_duty(led_rgb_ch[2].mode, led_rgb_ch[2].channel, blue);
	ledc_update_duty(led_rgb_ch[2].mode, led_rgb_ch[2].channel);
}

void led_rgb_wifi_app_started(void){
	if (g_pwn_init_handle == false) led_rgb_pwn_init();
	//led_rgb_set_color(255, 102, 255);
	led_rgb_set_color(0, 0, 0); // White
}


void led_rgb_http_server_started(void){
	if (g_pwn_init_handle == false) led_rgb_pwn_init();
	//led_rgb_set_color(204, 255, 51);
	led_rgb_set_color(28, 3, 252); // Yellow
}


void led_rgb_wifi_connected(void){
	if (g_pwn_init_handle == false) led_rgb_pwn_init();
	//led_rgb_set_color(0, 255, 153);
	led_rgb_set_color(255, 0, 255); // Green
}

void led_rgb_test(void){
	led_rgb_set_color(42, 250, 0); // Purple
}



