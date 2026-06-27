#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "portmacro.h"
#include "led.h"
#include "radar.h"

const TickType_t duration = 100 / portTICK_PERIOD_MS;

void app_main(void)
{



    static const char *ourTaskName = "main";
    ESP_LOGI(ourTaskName, "Hello world!");

   // led_init();
    void init_radar_gpio();

    while(1)
    {
        //led_toggle();
        //vTaskDelay(duration);
        void trigger_sensor();
        uint32_t time_echo = measure_echo();
        float meassure_distance = distance_cm(time_echo);
        printf("Distancia: %.2f cm\n", meassure_distance);

        vTaskDelay(pdMS_TO_TICKS(500));
    }

}
