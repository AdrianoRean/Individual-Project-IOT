#include "main.h"
#include <sys/time.h>

const char *TIME_TAG = "Time";
struct timeval tv;

static void printTime(void){
    setenv("TZ", "CEST+2", 1);
    tzset();

    gettimeofday(&tv, NULL);
    double time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
    ESP_LOGI(TIME_TAG, "The current date/time in Rome is: %lf", time_in_mill);
}