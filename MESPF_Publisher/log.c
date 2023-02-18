/*
 * log.c
 *
 *  Created on: 9 feb. 2023
 *      Author: fabri
 */

#include <string.h>
#include <log.h>
#include <status.h>

char log_buf[LOG_BUF_LEN];
int log_head = -1;

void log_add(const char *msg)
{
	char tm[20];
	int tm_len;
	status_getDateTime(tm);
	tm_len = strlen(tm);

	log_head = (log_head + 1) % (LOG_BUF_LEN / LOG_MSG_LEN);
	strncpy(&log_buf[log_head * LOG_MSG_LEN], tm, tm_len);
	strcat(&log_buf[log_head * LOG_MSG_LEN + tm_len], " ");
	strncat(&log_buf[log_head * LOG_MSG_LEN + tm_len + 1], msg, LOG_MSG_LEN - tm_len - 1);
}

void log_get(char *dst)
{
    int i = (log_head + 1) % (LOG_BUF_LEN / LOG_MSG_LEN);
    strncpy(dst, &log_buf[i * LOG_MSG_LEN], LOG_MSG_LEN);
    while (i != log_head) {
        i = (i + 1) % (LOG_BUF_LEN / LOG_MSG_LEN);
        strncat(dst, &log_buf[i * LOG_MSG_LEN], LOG_MSG_LEN);
    }
}



