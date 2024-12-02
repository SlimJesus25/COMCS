#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct{
    int hour;
    int minute;
    int second;
    int millisecond;
}timestamp_t;

void log_msg(char* message){
    if(message == NULL)
        return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[Thread-%ld] %d-%02d-%02d %02d:%02d:%02d %s\n", pthread_self(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, message);
}

void log_error(char* message){
    if(message == NULL)
        return;
    char msg[strlen(message) + strlen("[ERROR] ") + 1];
    sprintf(msg, "[ERROR] %s", message);
    log_msg(msg);
}

void log_info(char* message){
    if(message == NULL)
        return;
    char msg[strlen(message) + strlen("[INFO] ") + 1];
    sprintf(msg, "[INFO] %s", message);
    log_msg(msg);
}

void log_warning(char* message){
    if(message == NULL)
        return;
    char msg[strlen(message) + strlen("[WARNING] ") + 1];
    sprintf(msg, "[WARNING] %s", message);
    log_msg(msg);
}

void decode_date_observed(char* date_observed, timestamp_t* ts){
    char h[3], m[3], s[3], ms[3];

    strncpy(h, date_observed+11, 2);
    h[2] = '\0';
    strncpy(m, date_observed+14, 2);
    m[2] = '\0';
    strncpy(s, date_observed+17, 2);
    s[2] = '\0';
    strncpy(ms, date_observed+20, 2);
    ms[2] = '\0';

    ts->hour = atoi(h);
    ts->minute = atoi(m);
    ts->second = atoi(s);
    ts->millisecond = atoi(ms);
}

/**
 * Returns the time difference between ts1 and ts2 in milliseconds.
 */
long calculate_time_difference(timestamp_t ts1, timestamp_t ts2){
    long ms_diff = 0;
    ms_diff += abs(ts1.hour - ts2.hour) * 3600000;
    ms_diff += abs(ts1.minute - ts2.minute) * 60000;
    ms_diff += abs(ts1.second - ts2.second) * 1000;
    ms_diff += abs(ts1.millisecond - ts2.millisecond);
    return ms_diff;
}