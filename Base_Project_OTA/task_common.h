/*
 * task_common.h
 *
 *  Created on: 14 jun. 2022
 *      Author: fabri
 */

#ifndef MAIN_TASK_COMMON_H_
#define MAIN_TASK_COMMON_H_

#define WIFI_APP_TASK_STACK_SIZE		4096
#define WIFI_APP_TASK_PRIORITY			5
#define WIFI_APP_TASK_CORE_ID			0

#define HTTP_SERVER_TASK_STACK_SIZE		8192
#define HTTP_SERVER_TASK_PRIORITY		4
#define HTTP_SERVER_TASK_CORE_ID		0

#define HTTP_SERVER_MONITOR_STACK_SIZE	4096
#define HTTP_SERVER_MONITOR_PRIORITY	3
#define HTTP_SERVER_MONITOR_CORE_ID		0

#endif /* MAIN_TASK_COMMON_H_ */
