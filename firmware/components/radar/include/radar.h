#ifndef RADAR_H
#define RADAR_H

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct
{
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;
    QueueHandle_t echo_queue;

    volatile int64_t echo_start_time;
} radar_dev_t;

esp_err_t radar_init(radar_dev_t *dev);
esp_err_t radar_get_distance(radar_dev_t *dev, float *distance_cm);

#endif

