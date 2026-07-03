#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "radar.h"
#include "button.h"
#include "config.h"
#include "rtc.h"
#include "i2c_bus.h"
#include "lcd.h"

static const char *TAG = "main";

static void lcd_task(void *pvParameters)
{
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)pvParameters;

    esp_err_t ret = lcd_init(bus);
    if (ret != ESP_OK) {
        printf("Error inicializando LCD: %s\n", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    lcd_clear();

    lcd_set_cursor(0, 0);
    lcd_print("Hola mundo!");

    int contador = 0;
    char buf[24];

    while (1) {
        lcd_set_cursor(0, 1);

        contador %= 10000;
        snprintf(buf, sizeof(buf), "Contador: %4d", contador++);

        lcd_print(buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello world!");
    gpio_set_direction(ON_LED, GPIO_MODE_OUTPUT);
    radar_init();
    QueueHandle_t button_queue = button_init();

    bool radar_running = true;
    uint8_t dummy;
    i2c_master_bus_handle_t bus = i2c_bus_init();
    xTaskCreate(ds3231_test,
            "ds3231_test",
            configMINIMAL_STACK_SIZE * 3,
            bus,
            5,
            NULL);
 //   i2c_scan(bus);


    xTaskCreate(
        lcd_task,      // función
        "lcd_task",    // nombre
        4096,          // stack
        bus,           // parámetro
        5,             // prioridad
        NULL           // handle
    );

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
