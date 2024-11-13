#ifndef SMART_DATA_H
#define SMART_DATA_H

#include <stdio.h>

typedef struct{
    char *dateObserved;
    char *id;
    char *location;
    char *type;
    double temperature;
    double humidity;
    long timestamp;
} smart_data_t;

#endif