#include "radar.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"


static volatile int64_t echo_start_time = 0;
static float last_distance_cm = -1.0f;
static volatile bool is_paused = false;



static void IRAM_ATTR echo_isr_handler(void *arg)
{
    radar_dev_t *dev = (radar_dev_t *)arg;

    int64_t now = esp_timer_get_time();

    if (gpio_get_level(dev->echo_pin))
    {
        dev->echo_start_time = now;
    }
    else
    {
        int64_t duration = now - dev->echo_start_time;

        BaseType_t hp = pdFALSE;
        xQueueSendFromISR(dev->echo_queue, &duration, &hp);

        if (hp)
            portYIELD_FROM_ISR();
    }
}

static void trigger_sensor(radar_dev_t *dev)
{
    gpio_set_level(dev->trig_pin, 0);
    esp_rom_delay_us(2);

    gpio_set_level(dev->trig_pin, 1);
    esp_rom_delay_us(10);

    gpio_set_level(dev->trig_pin, 0);

}

esp_err_t radar_get_distance(radar_dev_t *dev,
                             float *distance_cm)
{
    int64_t pulse_duration;

    trigger_sensor(dev);

    if (xQueueReceive(dev->echo_queue,
                      &pulse_duration,
                      pdMS_TO_TICKS(60)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    *distance_cm = pulse_duration / 58.0f;

    return ESP_OK;
}

esp_err_t radar_init(radar_dev_t *dev)
{
    dev->trig_pin = TRIG_PIN;
    dev->echo_pin = ECHO_PIN;

    gpio_set_direction(dev->trig_pin, GPIO_MODE_OUTPUT);

    gpio_set_direction(dev->echo_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(dev->echo_pin, GPIO_PULLDOWN_ONLY);

    gpio_set_intr_type(dev->echo_pin, GPIO_INTR_ANYEDGE);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(dev->echo_pin,
                         echo_isr_handler,
                         dev);

    dev->echo_queue = xQueueCreate(1, sizeof(int64_t));

    return dev->echo_queue ? ESP_OK : ESP_ERR_NO_MEM;
}


void radar_set_paused(bool paused)
{
    is_paused = paused;
}

float radar_get_last_distance_cm(void)
{
    return last_distance_cm;
}


