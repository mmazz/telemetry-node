#ifndef RADAR_H
#define RADAR_H
#include "driver/gpio.h"
#include <stdint.h>
#include "config.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

void init_radar_gpio();
void trigger_sensor();
uint32_t measure_echo();
float distance_cm(uint32_t time_us);

#endif

