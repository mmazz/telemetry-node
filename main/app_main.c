#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "portmacro.h"
#include "led.h"

const TickType_t duration = 100 / portTICK_PERIOD_MS;

void app_main(void)
{

    static const char *ourTaskName = "main";
    ESP_LOGI(ourTaskName, "Hello world!");

    led_init();

    while(1)
    {
        led_toggle();
        vTaskDelay(duration);
    }

}
