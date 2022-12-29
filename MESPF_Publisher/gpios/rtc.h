/*
 * rtc2.h
 *
 *  Created on: 20 dic. 2022
 *      Author: fabri
 */

#ifndef MAIN_GPIOS_RTC2_H_
#define MAIN_GPIOS_RTC2_H_

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
 * @defgroup ds1302 ds1302
 * @{
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
#include <stdbool.h>
#include <driver/gpio.h>
#include <time.h>
#include <esp_err.h>
#include "recollecter.h"

#define DS1302_CLK_GPIO		25	// CLK
#define DS1302_IO_GPIO		32	// DAT
#define DS1302_CE_GPIO		33 	// RST


#define DS1302_RAM_SIZE 31

/**
 * Device descriptor
 */
typedef struct
{
    gpio_num_t ce_pin;     //!< GPIO pin connected to CE
    gpio_num_t io_pin;     //!< GPIO pin connected to chip I/O
    gpio_num_t sclk_pin;   //!< GPIO pin connected to SCLK
    bool ch;               //!< true if clock is halted
} ds1302_t;

bool rtc_initialize();
bool rtc_setDateTime(const struct tm *time);
bool rtc_getDateTime(struct tm *time);

sensor_data_t rtc_recollecter (void);



#endif /* MAIN_GPIOS_RTC2_H_ */
