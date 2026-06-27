#include "radar.h"

void init_radar_gpio()
{
    gpio_set_direction(
        TRIG_PIN,
        GPIO_MODE_OUTPUT
    );


    gpio_set_direction(
        ECHO_PIN,
        GPIO_MODE_INPUT
    );
}

void trigger_sensor()
{
    gpio_set_level(TRIG_PIN, 0);

    esp_rom_delay_us(2);


    gpio_set_level(TRIG_PIN, 1);

    esp_rom_delay_us(10);


    gpio_set_level(TRIG_PIN, 0);
}

uint32_t measure_echo()
{
    int64_t timeout;


    timeout = esp_timer_get_time();

    while(gpio_get_level(ECHO_PIN) == 0)
    {
        if(esp_timer_get_time() - timeout > 30000)
        {
            return 0;
        }

    }


    int64_t start = esp_timer_get_time();

    timeout = esp_timer_get_time();

    while(gpio_get_level(ECHO_PIN) == 1)
    {
        if(esp_timer_get_time() - timeout > 30000)
        {
            return 0;
        }
    }


    int64_t end = esp_timer_get_time();


    return end-start;
}

float distance_cm(uint32_t time_us)
{
    return (time_us * 0.0343) / 2;
}
