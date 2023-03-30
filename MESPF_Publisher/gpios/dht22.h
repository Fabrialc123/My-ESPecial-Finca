/*
 * dht22.h
 *
 *  Created on: 19 oct. 2022
 *      Author: Kike
 */

/**
 * [Note] DHT22 Documentation:
 *
 * DATA: Hum = 16 bits, Temp = 16 Bits, check-sum = 8 Bits
 *
 * Example: MCU has received 40 bits data from AM2302 as
 * 0000 0010 1000 1100 0000 0001 0101 1111 1110 1110
 * 16 bits RH data + 16 bits T data + check sum
 *
 * 1) we convert 16 bits RH data from binary system to decimal system, 0000 0010 1000 1100 → 652
 * Binary system Decimal system: RH=652/10=65.2%RH
 *
 * 2) we convert 16 bits T data from binary system to decimal system, 0000 0001 0101 1111 → 351
 * Binary system Decimal system: T=351/10=35.1°C
 *
 * When highest bit of temperature is 1, it means the temperature is below 0 degree Celsius.
 * Example: 1000 0000 0110 0101, T= minus 10.1°C: 16 bits T data
 *
 * 3) Check Sum=0000 0010+1000 1100+0000 0001+0101 1111=1110 1110 Check-sum=the last 8 bits of Sum=11101110
 *
 * Signal & Timings:
 *
 * The interval of whole process must be beyond 2 seconds.
 *
 * To request data from DHT:
 *
 * 1) Sent low pulse for > 1~10 ms (MILI SEC)
 * 2) Sent high pulse for > 20~40 us (Micros).
 * 3) When DHT detects the start signal, it will pull low the bus 80us as response signal,
 *    then the DHT pulls up 80us for preparation to send data.
 * 4) When DHT is sending data to MCU, every bit's transmission begin with low-voltage-level that last 50us,
 *    the following high-voltage-level signal's length decide the bit is "1" or "0".
 *    0: 26~28 us
 *    1: 70 us
 */

#ifndef MAIN_GPIOS_DHT22_H_
#define MAIN_GPIOS_DHT22_H_

#include "recollecter.h"

// ------------------------------------------------- Sensor information --------------------------------------------------------------

// Defines

#define DHT22_N_GPIOS						1

#define DHT22_N_VALUES						2

#define DHT22_N_ADDITIONAL_PARAMS			0

#define DHT22_MAX_DATA 						5

#define DHT22_TIME_TO_UPDATE_DATA			500 // 100 = 1 second

// Structure for sensor GPIOS

typedef struct {
	int data;
} dht22_gpios_t;

// Structure for sensor data

typedef struct {
	float humidity;
	float temperature;
} dht22_data_t;

// Values related to the LCD

#define DHT22_SHOW_HUMIDITY_ON_LCD			true

#define DHT22_SHOW_TEMPERATURE_ON_LCD		true

// --------------------------------------------------- Sensor management -----------------------------------------------------------

/*
 * Startup DHT22 with the information previously saved (NVS)
 */
void dht22_startup(void);

/*
 * Initializes DTH22 peripheral
 */
void dht22_init(void);

/*
 * Disables DTH22 peripheral
 */
void dht22_destroy(void);

/**
 * Add a new DHT22 sensor
 *
 * NOTE: return 1 when it happens successfully, -1 in other cases
 */
int dht22_add_sensor(int* gpios, union sensor_value_u* parameters);

/**
 * Delete a DHT22 sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int dht22_delete_sensor(int pos);

// ------------------------------------------------------- Sets --------------------------------------------------------------------

/**
 * Change GPIOS of a sensor
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int dht22_set_gpios(int pos, int* gpios);

/**
 * Change Parameters of a sensor
 *
 * Note: return 1  when it happens successfully, -1 in other cases
 */
int dht22_set_parameters(int pos, union sensor_value_u* parameters);

/**
 * Change the alert values
 *
 * Note: return 1 when it happens successfully, -1 in other cases
 */
int dht22_set_alert_values(int value, bool alert, int n_ticks, union sensor_value_u upper_threshold, union sensor_value_u lower_threshold);

// ------------------------------------------------------- Gets --------------------------------------------------------------------

/**
 * Gets data of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_data_t* dht22_get_sensors_data(int* number_of_sensors);

/**
 * Gets GPIOS of all sensors of this type
 *
 * Note: in case of error, return NULL pointer
 */
sensor_gpios_info_t* dht22_get_sensors_gpios(int* number_of_sensors);

/**
 * Gets additional parameters of all sensors of this type
 *
 * Note: in case of error (or it is a sensor with no additional parameters), return NULL pointer
 */
sensor_additional_parameters_info_t* dht22_get_sensors_additional_parameters(int* number_of_sensors);

#endif /* MAIN_GPIOS_DHT22_H_ */
