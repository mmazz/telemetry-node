#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "portmacro.h"
#include "led.h"

#define BLINK_LED 2
void app_main(void)
{

    char *ourTaskName = pcTaskGetName(NULL);
    ESP_LOGI(ourTaskName, "Hello world!");

    led_init();

    while(1)
    {
        led_on();
        led_off();
    }

}
