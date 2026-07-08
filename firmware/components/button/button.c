#include "button.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "Button";
static QueueHandle_t button_queue;
static volatile int64_t last_press_time = 0;
static volatile uint32_t isr_count = 0;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();
    if (now - last_press_time < DEBOUNCE_US)
    {
        return;
    }
    last_press_time = now;
    isr_count++;
    uint8_t dummy = 1;
    BaseType_t higher_priority_woken = pdFALSE;
    xQueueSendFromISR(button_queue, &dummy, &higher_priority_woken);
    if (higher_priority_woken)
    {
        portYIELD_FROM_ISR();
    }
}

uint32_t button_get_isr_count(void)
{
    return isr_count;
}

QueueHandle_t button_init(void)
{
    button_queue = xQueueCreate(5, sizeof(uint8_t));
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);

    // Pull-up interno: el pin esta en 1 en reposo, y baja a 0 al presionar.
    // Por eso usamos GPIO_INTR_NEGEDGE (flanco descendente = press).
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);

    esp_err_t err = gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_isr_handler_add falló: %s", esp_err_to_name(err));
    }
    return button_queue;
}
