#include "rtc.h"
#include "config.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char *TAG = "rtc";

#define DS3231_ADDR      0x68
#define DS3231_REG_TIME  0x00   /* seg, min, hora, dow, dia, mes, año */
#define DS3231_REG_TEMP  0x11   /* MSB (int8) + LSB (2 bits fraccion) */

static inline uint8_t dec2bcd(uint8_t val)
{
    return (uint8_t)(((val / 10) << 4) | (val % 10));
}

static inline uint8_t bcd2dec(uint8_t val)
{
    return (uint8_t)(((val >> 4) * 10) + (val & 0x0F));
}

esp_err_t rtc_ds_init(i2c_master_bus_handle_t bus, rtc_dev_t *dev)
{
    i2c_device_config_t cfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = 100000,
    };

    return i2c_master_bus_add_device(bus, &cfg, &dev->handle);
}

esp_err_t rtc_set_time(rtc_dev_t *dev, const struct tm *time)
{
    uint8_t buf[8];

    buf[0] = DS3231_REG_TIME;
    buf[1] = dec2bcd((uint8_t)time->tm_sec);
    buf[2] = dec2bcd((uint8_t)time->tm_min);
    buf[3] = dec2bcd((uint8_t)time->tm_hour);          /* bit6=0 -> modo 24h */
    buf[4] = dec2bcd((uint8_t)(time->tm_wday == 0 ? 7 : time->tm_wday)); /* DS3231 usa 1-7 */
    buf[5] = dec2bcd((uint8_t)time->tm_mday);
    buf[6] = dec2bcd((uint8_t)(time->tm_mon + 1));
    buf[7] = dec2bcd((uint8_t)(time->tm_year % 100));

    return i2c_master_transmit(dev->handle, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

esp_err_t rtc_get_time(rtc_dev_t *dev, struct tm *time)
{
    uint8_t reg = DS3231_REG_TIME;
    uint8_t buf[7];

    esp_err_t err = i2c_master_transmit_receive(dev->handle,
                                                 &reg, 1,
                                                 buf, sizeof(buf),
                                                 pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }

    time->tm_sec  = bcd2dec(buf[0] & 0x7F);
    time->tm_min  = bcd2dec(buf[1] & 0x7F);
    time->tm_hour = bcd2dec(buf[2] & 0x3F);   /* asume modo 24h (bit6=0) */
    time->tm_wday = bcd2dec(buf[3] & 0x07) % 7;
    time->tm_mday = bcd2dec(buf[4] & 0x3F);
    time->tm_mon  = bcd2dec(buf[5] & 0x1F) - 1;
    time->tm_year = bcd2dec(buf[6]) + 100;     /* años desde 1900, asume 20xx */

    return ESP_OK;
}

esp_err_t rtc_get_temp(rtc_dev_t *dev, float *temp_c)
{
    uint8_t reg = DS3231_REG_TEMP;
    uint8_t buf[2];

    esp_err_t err = i2c_master_transmit_receive(dev->handle,
                                                 &reg, 1,
                                                 buf, sizeof(buf),
                                                 pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }

    int8_t msb = (int8_t)buf[0];
    *temp_c = (float)msb + ((buf[1] >> 6) * 0.25f);

    return ESP_OK;
}

void ds3231_test(void *pvParameters)
{
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)pvParameters;
    rtc_dev_t dev;

    ESP_ERROR_CHECK(rtc_ds_init(bus, &dev));

    struct tm time =
    {
        .tm_year = 122,   /* 2022 - 1900 */
        .tm_mon  = 11,    /* 0-based -> diciembre */
        .tm_mday = 15,
        .tm_hour = 0,
        .tm_min  = 50,
        .tm_sec  = 10,
        .tm_wday = 4,     /* jueves, ajustalo si hace falta */
    };
    ESP_ERROR_CHECK(rtc_set_time(&dev, &time));

    while (1)
    {
        float temp_c;

        vTaskDelay(pdMS_TO_TICKS(250));

        if (rtc_get_temp(&dev, &temp_c) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo leer la temperatura");
            continue;
        }

        if (rtc_get_time(&dev, &time) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo leer la hora");
            continue;
        }

        ESP_LOGI(TAG, "%04d-%02d-%02d %02d:%02d:%02d, %.2f degC",
                 time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                 time.tm_hour, time.tm_min, time.tm_sec, temp_c);
    }
}
