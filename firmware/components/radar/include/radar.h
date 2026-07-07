#ifndef RADAR_H
#define RADAR_H

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/queue.h"

typedef struct
{
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;

    QueueHandle_t echo_queue;

    volatile int64_t echo_start_time;
    volatile bool paused;
} radar_dev_t;

esp_err_t radar_init(radar_dev_t *dev);

esp_err_t radar_get_distance(radar_dev_t *dev, float *distance_cm);
float radar_get_last_distance_cm(void);
void radar_set_paused(bool paused);

#endif

