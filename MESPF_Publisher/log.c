/*
 * log.c
 *
 *  Created on: 9 feb. 2023
 *      Author: fabri
 */

#include <string.h>
#include <log.h>

char log_buf[LOG_BUF_LEN];
int log_head = -1;

void log_add(const char *msg)
{
   log_head = (log_head + 1) % (LOG_BUF_LEN / LOG_MSG_LEN);
   strncpy(&log_buf[log_head * LOG_MSG_LEN], msg, LOG_MSG_LEN );
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



