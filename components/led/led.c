#include "led.h"
#include "config.h"



void led_init(void) {
    gpio_reset_pin(BLINK_LED);
    gpio_set_direction(BLINK_LED, GPIO_MODE_OUTPUT);
}

void led_on(void) {
    gpio_set_level(BLINK_LED, 1);
}

void led_off(void) {
    gpio_set_level(BLINK_LED, 0);
}

void led_toggle(void) {
    int level = gpio_get_level(BLINK_LED);
    gpio_set_level(BLINK_LED, !level);
}

