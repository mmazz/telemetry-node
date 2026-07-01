#include "reloj.h"
#include "config.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

void ds3231_test(void *pvParameters)
{
    i2c_dev_t dev;
    memset(&dev, 0, sizeof(i2c_dev_t));

    ESP_ERROR_CHECK(ds3231_init_desc(&dev, 0, I2C_SDA, I2C_SDL));

    struct tm time = {
        .tm_year = 122, // (2022 - 1900)
        .tm_mon  = 11,  // 0-based
        .tm_mday = 15,
        .tm_hour = 0,
        .tm_min  = 50,
        .tm_sec  = 10
    };
    ESP_ERROR_CHECK(ds3231_set_time(&dev, &time));

    while (1)
    {
        float temp;

        vTaskDelay(pdMS_TO_TICKS(250));

        if (ds3231_get_temp_float(&dev, &temp) != ESP_OK)
        {
            printf("Could not get temperature\n");
            continue;
        }

        if (ds3231_get_time(&dev, &time) != ESP_OK)
        {
            printf("Could not get time\n");
            continue;
        }

        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        printf("%04d-%02d-%02d %02d:%02d:%02d, %.2f deg Cel\n", time.tm_year + 1900 /*Add 1900 for better readability*/, time.tm_mon + 1,
            time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, temp);
    }
}

