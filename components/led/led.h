#pragma once
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "config.h"
void led_init(void);
void led_on(void);
void led_off(void);
void led_toggle(void);
