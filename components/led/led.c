#include "led.h"


#define BLINK_LED 16
const TickType_t duration = 500 / portTICK_PERIOD_MS;


void led_init(void){
    gpio_reset_pin(BLINK_LED);
    gpio_set_direction(BLINK_LED, GPIO_MODE_OUTPUT);
}

void led_on(void){
    gpio_set_level(BLINK_LED, 1);
    vTaskDelay(duration);
}
void led_off(void){
    gpio_set_level(BLINK_LED, 0);
    vTaskDelay(duration);
}
void led_toggle(void);
