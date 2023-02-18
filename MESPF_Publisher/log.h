/*
 * log.h
 *
 *  Created on: 9 feb. 2023
 *      Author: fabri
 */

#ifndef MAIN_LOG_H_
#define MAIN_LOG_H_

#define LOG_BUF_LEN 3000
#define LOG_MSG_LEN 100

void log_add(const char *msg);

void log_get(char *dst);

#endif /* MAIN_LOG_H_ */
