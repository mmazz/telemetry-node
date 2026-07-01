#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "radar.h"
#include "button.h"
#include "config.h"
#include "reloj.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");
    gpio_set_direction(ON_LED, GPIO_MODE_OUTPUT);
    radar_init();
    QueueHandle_t button_queue = button_init();

    bool radar_running = true;
    uint8_t dummy;
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreate(ds3231_test, "ds3231_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    while (1)
    {

        bool toggle_requested = false;
        while (xQueueReceive(button_queue, &dummy, 0) == pdTRUE)
        {
            toggle_requested = true; // hubo al menos un evento -> togglear una vez
        }
        if (toggle_requested)
        {
            radar_running = !radar_running;
            gpio_set_level(ON_LED, radar_running ? 1 : 0);
            radar_set_paused(!radar_running);
            ESP_LOGI(TAG, "%s", radar_running ? "Radar reanudado" : "Radar pausado");

        }

        if (radar_running)
        {
            float d = radar_get_last_distance_cm();

            if (d >= 0)
            {
                ESP_LOGI(TAG, "Distancia: %.2f cm", d);
            }
            else
            {
                ESP_LOGI(TAG, "Sin lectura valida");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
