#include "radar.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"


static QueueHandle_t echo_queue;
static volatile int64_t echo_start_time = 0;
static float last_distance_cm = -1.0f;
static volatile bool is_paused = false;
// --- ISR: se ejecuta en contexto de interrupcion, debe ser RAPIDA ---
static void IRAM_ATTR echo_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();

    if (gpio_get_level(ECHO_PIN) == 1)
    {
        // Flanco ascendente: arranca el pulso
        echo_start_time = now;
    }
    else
    {
        // Flanco descendente: termino el pulso
        int64_t duration = now - echo_start_time;

        BaseType_t higher_priority_woken = pdFALSE;
        xQueueSendFromISR(echo_queue, &duration, &higher_priority_woken);

        if (higher_priority_woken)
        {
            portYIELD_FROM_ISR();
        }
    }
}

static void trigger_sensor(void)
{
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);
}

static void radar_task(void *pvParameters)
{
    int64_t pulse_duration;

    while (1)
    {
        if (is_paused)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue; // se salta la medicion completa este ciclo
        }

        trigger_sensor();

        if (xQueueReceive(echo_queue, &pulse_duration, pdMS_TO_TICKS(60)) == pdTRUE)
        {
            last_distance_cm = pulse_duration / 58.0f;
        }
        else
        {
            last_distance_cm = -1.0f;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void radar_set_paused(bool paused)
{
    is_paused = paused;
}

void radar_init(void)
{
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);

    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ECHO_PIN, GPIO_PULLDOWN_ONLY);

    gpio_set_intr_type(ECHO_PIN, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ECHO_PIN, echo_isr_handler, NULL);

    echo_queue = xQueueCreate(1, sizeof(int64_t));

    xTaskCreate(radar_task, "radar_task", 2048, NULL, 5, NULL);
}

float radar_get_last_distance_cm(void)
{
    return last_distance_cm;
}
