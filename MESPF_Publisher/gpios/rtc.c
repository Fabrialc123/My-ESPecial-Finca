/*
 * rtc2.c
 *
 *  Created on: 20 dic. 2022
 *      Author: fabri
 */


/*
 * Copyright (c) 2016 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (c) 2016 Pavel Merzlyakov <merzlyakovpavel@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of itscontributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ds1302.h
 *
 * ESP-IDF driver for DS1302 RTC
 *
 * Ported from esp-open-rtos
 *
 * Copyright (c) 2016 Ruslan V. Uss <unclerus@gmail.com>\n
 * Copyright (c) 2016 Pavel Merzlyakov <merzlyakovpavel@gmail.com>
 *
 * BSD Licensed as described in the file LICENSE
 */
#include <freertos/FreeRTOS.h>
#include <string.h>
#include "rom/ets_sys.h"
#include "esp_idf_lib_helpers.h"
#include "rtc.h"
#include <pthread.h>
#include "esp_log.h"

#include "../recollecter.h"

#define CH_REG   0x80
#define WP_REG   0x8e

#define CH_BIT     (1 << 7)
#define WP_BIT     (1 << 7)
#define HOUR12_BIT (1 << 7)
#define PM_BIT     (1 << 5)

#define CH_MASK ((uint8_t)(~CH_BIT))
#define WP_MASK ((uint8_t)(~WP_BIT))

#define CLOCK_BURST 0xbe
#define RAM_BURST   0xfe

#define SECONDS_MASK 0x7f
#define HOUR12_MASK  0x1f
#define HOUR24_MASK  0x3f

#define GPIO_BIT(x) (1ULL << (x))

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#if HELPER_TARGET_IS_ESP32
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define PORT_ENTER_CRITICAL portENTER_CRITICAL(&mux)
#define PORT_EXIT_CRITICAL portEXIT_CRITICAL(&mux)
#else
#define PORT_ENTER_CRITICAL portENTER_CRITICAL()
#define PORT_EXIT_CRITICAL portEXIT_CRITICAL()
#endif

#define CHECK_MUX(x) do { esp_err_t __; if ((__ = (x)) != ESP_OK) { PORT_EXIT_CRITICAL; return __; } } while (0)

static const char TAG[] = "RTC(DS1302)";

static ds1302_t dev;

static pthread_mutex_t mutex_RTC;
bool rtc_initialized 	= false;

/**
 * @brief Initialize device
 *
 * @param dev Device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_init(ds1302_t *dev);

/**
 * @brief Start/stop clock
 *
 * @param dev Device descriptor
 * @param start Start clock if true
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_start(ds1302_t *dev, bool start);

/**
 * @brief Get current clock state
 *
 * @param dev Device descriptor
 * @param[out] running true if clock running
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_is_running(ds1302_t *dev, bool *running);

/**
 * @brief Enable/disable write protection
 *
 * @param dev Device descriptor
 * @param wp Set RTC write-protected if true
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_set_write_protect(ds1302_t *dev, bool wp);

/**
 * @brief Get write protection status
 *
 * @param dev Device descriptor
 * @param[out] wp true if RTC write-protected
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_get_write_protect(ds1302_t *dev, bool *wp);

/**
 * @brief Get current time
 *
 * @param dev Device descriptor
 * @param[out] time Current time
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_get_time(ds1302_t *dev, struct tm *time);

/**
 * @brief Set time to RTC
 *
 * @param dev Device descriptor
 * @param time Time
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_set_time(ds1302_t *dev, const struct tm *time);

/**
 * @brief Read RAM contents into the buffer
 *
 * @param dev Device descriptor
 * @param offset Start byte, 0..55
 * @param[out] buf Buffer
 * @param len Bytes to read, 1..56
 *
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_read_sram(ds1302_t *dev, uint8_t offset, void *buf, uint8_t len);

/**
 * @brief Write buffer to RTC RAM
 *
 * @param dev Device descriptor
 * @param offset Start byte, 0..55
 * @param buf Buffer
 * @param len Bytes to write, 1..56
 *
 * @return `ESP_OK` on success
 */
esp_err_t ds1302_write_sram(ds1302_t *dev, uint8_t offset, void *buf, uint8_t len);

static uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

static uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

inline static esp_err_t chip_enable(ds1302_t *dev)
{
    CHECK(gpio_set_level(dev->ce_pin, 1));
    ets_delay_us(4);
    return ESP_OK;
}

inline static esp_err_t chip_disable(ds1302_t *dev)
{
    return gpio_set_level(dev->ce_pin, 0);
}

inline static esp_err_t prepare(ds1302_t *dev, gpio_mode_t mode)
{
    CHECK(gpio_set_direction(dev->io_pin, mode));
    CHECK(gpio_set_level(dev->sclk_pin, 0));
    return chip_enable(dev);
}

inline static esp_err_t toggle_clock(ds1302_t *dev)
{
    CHECK(gpio_set_level(dev->sclk_pin, 1));
    ets_delay_us(1);
    CHECK(gpio_set_level(dev->sclk_pin, 0));
    ets_delay_us(1);
    return ESP_OK;
}

static esp_err_t write_byte(ds1302_t *dev, uint8_t b)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        CHECK(gpio_set_level(dev->io_pin, (b >> i) & 1));
        CHECK(toggle_clock(dev));
    }
    return ESP_OK;
}

static esp_err_t read_byte(ds1302_t *dev, uint8_t *b)
{
    *b = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        *b |= gpio_get_level(dev->io_pin) << i;
        CHECK(toggle_clock(dev));
    }
    return ESP_OK;
}

static esp_err_t read_register(ds1302_t *dev, uint8_t reg, uint8_t *val)
{
    PORT_ENTER_CRITICAL;
    CHECK_MUX(prepare(dev, GPIO_MODE_OUTPUT));
    CHECK_MUX(write_byte(dev, reg | 0x01));
    CHECK_MUX(prepare(dev, GPIO_MODE_INPUT));
    CHECK_MUX(read_byte(dev, val));
    PORT_EXIT_CRITICAL;
    return chip_disable(dev);
}

static esp_err_t write_register(ds1302_t *dev, uint8_t reg, uint8_t val)
{
    PORT_ENTER_CRITICAL;
    CHECK_MUX(prepare(dev, GPIO_MODE_OUTPUT));
    CHECK_MUX(write_byte(dev, reg));
    CHECK_MUX(write_byte(dev, val));
    PORT_EXIT_CRITICAL;
    return chip_disable(dev);
}

static esp_err_t burst_read(ds1302_t *dev, uint8_t reg, uint8_t *dst, uint8_t len)
{
    PORT_ENTER_CRITICAL;
    CHECK_MUX(prepare(dev, GPIO_MODE_OUTPUT));
    CHECK_MUX(write_byte(dev, reg | 0x01));
    CHECK_MUX(prepare(dev, GPIO_MODE_INPUT));
    for (uint8_t i = 0; i < len; i++, dst++)
        CHECK_MUX(read_byte(dev, dst));
    PORT_EXIT_CRITICAL;
    return chip_disable(dev);
}

static esp_err_t burst_write(ds1302_t *dev, uint8_t reg, uint8_t *src, uint8_t len)
{
    PORT_ENTER_CRITICAL;
    CHECK_MUX(prepare(dev, GPIO_MODE_OUTPUT));
    CHECK_MUX(write_byte(dev, reg));
    for (uint8_t i = 0; i < len; i++, src++)
        CHECK_MUX(write_byte(dev, *src));
    PORT_EXIT_CRITICAL;
    return chip_disable(dev);
}

static esp_err_t update_register(ds1302_t *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
    uint8_t r;
    CHECK(read_register(dev, reg, &r));
    return write_register(dev, reg, (r & mask) | val);
}

/*------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------
 *------------------------------------------------------------------------------------------------------
 */

sensor_data_t rtc_recollecter (void){
	sensor_data_t aux;
	sensor_value_t *aux2;
	struct tm timeInfo;

	char *time, *date;

	int number_of_values = 2;

	time = (char*)malloc(sizeof(char)*40);
	date = (char*)malloc(sizeof(char)*40);

	memset(time,0,40);
	memset(date,0,40);

	if(!rtc_getDateTime(&timeInfo)){
		ESP_LOGE(TAG,"get_DateTime failed!");
		strcpy(time, "-1");
		strcpy(date, "-1");
	}
	else {
		sprintf(time, "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
		sprintf(date, "%02d/%02d/%04d", timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);
	}

	aux2 = (sensor_value_t *)malloc(sizeof(sensor_value_t) * number_of_values);

	strcpy(aux.sensorName, "RTC");
	aux.valuesLen = number_of_values;
	aux.sensor_values = aux2;
	aux.sensor_values[0].sensor_value_type = STRING;
	strcpy(aux.sensor_values[0].valueName,"INTEGERTEST");
	strcpy(aux.sensor_values[0].sensor_value.cval,time );

	aux.sensor_values[1].sensor_value_type = STRING;
	strcpy(aux.sensor_values[1].valueName,"FLOATTEST");
	strcpy(aux.sensor_values[1].sensor_value.cval, date);

	free(time);
	free(date);

	return aux;
}

bool rtc_initialize(){
	struct tm timeInfo;
	char strftime_buf[64];

	if (rtc_initialized){
		ESP_LOGE(TAG, "RTC already initialized!");
		return rtc_initialized;
	}

	if(pthread_mutex_init (&mutex_RTC, NULL) != 0){
		 ESP_LOGE(TAG,"Failed to initialize the RTC mutex");
		 return false;
	}

	dev.ce_pin = DS1302_CE_GPIO;
	dev.io_pin = DS1302_IO_GPIO;
	dev.sclk_pin = DS1302_CLK_GPIO;
	dev.ch = false;

	if(ds1302_init(&dev) != ESP_OK){
		ESP_LOGE(TAG, "RTC init failed!");
		return false;
	}

	if (ds1302_start(&dev, true) != ESP_OK){
		ESP_LOGE(TAG, "RTC start failed!");
		return false;
	}

	if (ds1302_get_time(&dev,&timeInfo) != ESP_OK){
		ESP_LOGE(TAG, "RTC get_time failed!");
		return false;
	}

	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeInfo);
	ESP_LOGI(TAG,"RTC initiliazed with %s", strftime_buf);


	rtc_initialized = true;

	return true;
}
bool rtc_setDateTime(const struct tm *time ){
	char strftime_buf[64];
	struct tm timeAux;

	if (!rtc_initialized){
		ESP_LOGE(TAG, "RTC is not initialized!");
		return false;
	}
	pthread_mutex_lock(&mutex_RTC);

	if (ds1302_set_time(&dev,time) != ESP_OK){
		ESP_LOGE(TAG, "RTC set_time failed!");
		return false;
	}

	if (ds1302_get_time(&dev,&timeAux) != ESP_OK){
		ESP_LOGE(TAG, "RTC get_time failed in rtc_setDateTime!");
		return false;
	}

	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeAux);
	ESP_LOGI(TAG,"Succesfully set date time %s", strftime_buf);

	pthread_mutex_unlock(&mutex_RTC);

	return true;

}

bool rtc_getDateTime(struct tm *time){
	if (!rtc_initialized){
		ESP_LOGE(TAG, "RTC is not initialized!");
		return false;
	}

	pthread_mutex_lock(&mutex_RTC);
	if (ds1302_get_time(&dev, time) != ESP_OK){
		ESP_LOGE(TAG, "RTC get_time failed!");
		return false;
	}
	pthread_mutex_unlock(&mutex_RTC);

	return true;
}




esp_err_t ds1302_init(ds1302_t *dev)
{
    CHECK_ARG(dev);

    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(gpio_config_t));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =
            GPIO_BIT(dev->ce_pin) |
            GPIO_BIT(dev->io_pin) |
            GPIO_BIT(dev->sclk_pin);
    CHECK(gpio_config(&io_conf));

    bool r;
    CHECK(ds1302_is_running(dev, &r));
    dev->ch = !r;

    return ESP_OK;
}

esp_err_t ds1302_start(ds1302_t *dev, bool start)
{
    CHECK_ARG(dev);

    CHECK(update_register(dev, CH_REG, CH_MASK, start ? 0 : CH_BIT));
    dev->ch = !start;

    return ESP_OK;
}

esp_err_t ds1302_is_running(ds1302_t *dev, bool *running)
{
    CHECK_ARG(dev && running);

    uint8_t r;
    CHECK(read_register(dev, CH_REG, &r));
    *running = !(r & CH_BIT);
    dev->ch = !*running;

    return ESP_OK;
}

esp_err_t ds1302_set_write_protect(ds1302_t *dev, bool wp)
{
    CHECK_ARG(dev);

    return update_register(dev, WP_REG, WP_MASK, wp ? WP_BIT : 0);
}

esp_err_t ds1302_get_write_protect(ds1302_t *dev, bool *wp)
{
    CHECK_ARG(dev && wp);

    uint8_t r;
    CHECK(read_register(dev, WP_REG, &r));
    *wp = (r & WP_BIT) != 0;

    return ESP_OK;
}

esp_err_t ds1302_get_time(ds1302_t *dev, struct tm *time)
{
    CHECK_ARG(dev && time);

    uint8_t buf[7];
    CHECK(burst_read(dev, CLOCK_BURST, buf, 7));

    time->tm_sec = bcd2dec(buf[0] & SECONDS_MASK);
    time->tm_min = bcd2dec(buf[1]);
    if (buf[2] & HOUR12_BIT)
    {
        // RTC in 12-hour mode
        time->tm_hour = bcd2dec(buf[2] & HOUR12_MASK) - 1;
        if (buf[2] & PM_BIT)
            time->tm_hour += 12;
    }
    else time->tm_hour = bcd2dec(buf[2] & HOUR24_MASK);
    time->tm_mday = bcd2dec(buf[3]);
    time->tm_mon  = bcd2dec(buf[4]) - 1;
    time->tm_wday = bcd2dec(buf[5]) - 1;
    time->tm_year = bcd2dec(buf[6]) + 100;

    return ESP_OK;
}

esp_err_t ds1302_set_time(ds1302_t *dev, const struct tm *time)
{
    CHECK_ARG(dev && time);

    uint8_t buf[8] = {
        dec2bcd(time->tm_sec) | (dev->ch ? CH_BIT : 0),
        dec2bcd(time->tm_min),
        dec2bcd(time->tm_hour),
        dec2bcd(time->tm_mday),
        dec2bcd(time->tm_mon  + 1),
        dec2bcd(time->tm_wday + 1),
        dec2bcd(time->tm_year - 100),
        0
    };
    return burst_write(dev, CLOCK_BURST, buf, 8);
}

esp_err_t ds1302_read_sram(ds1302_t *dev, uint8_t offset, void *buf, uint8_t len)
{
    CHECK_ARG(dev && buf && len);
    CHECK_ARG(offset + len <= DS1302_RAM_SIZE);

    return burst_read(dev, RAM_BURST, (uint8_t *)buf, len);
}

esp_err_t ds1302_write_sram(ds1302_t *dev, uint8_t offset, void *buf, uint8_t len)
{
    CHECK_ARG(dev && buf && len);
    CHECK_ARG(offset + len <= DS1302_RAM_SIZE);

    return burst_write(dev, RAM_BURST, (uint8_t *)buf, len);
}

