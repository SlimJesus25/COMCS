#include <stdio.h>
#include <string.h>
#include <time.h>


void log_msg(char* message){
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%d-%02d-%02d %02d:%02d:%02d %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, message);
}

void log_error(char* message){
    char msg[strlen(message) + 7];
    sprintf(msg, "[ERROR] %s", message);
    log_msg(msg);
}

void log_info(char* message){
    char msg[strlen(message) + 6];
    sprintf(msg, "[INFO] %s", message);
    log_msg(msg);
}

void log_warning(char* message){
    char msg[strlen(message) + 9];
    sprintf(msg, "[WARNING] %s", message);
    log_msg(msg);
}