#ifndef RTC_H
#define RTC_H

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <time.h>

/* Handle del dispositivo DS3231 sobre el bus I2C nuevo (driver.i2c_master). */
typedef struct
{
    i2c_master_dev_handle_t handle;
} rtc_dev_t;

/* Registra el DS3231 como dispositivo del bus ya inicializado
 * (el mismo i2c_master_bus_handle_t que devuelve i2c_bus_init()). */
esp_err_t rtc_ds_init(i2c_master_bus_handle_t bus, rtc_dev_t *dev);

esp_err_t rtc_set_time(rtc_dev_t *dev, const struct tm *time);
esp_err_t rtc_get_time(rtc_dev_t *dev, struct tm *time);
esp_err_t rtc_get_temp(rtc_dev_t *dev, float *temp_c);

/* Tarea de prueba: recibe el i2c_master_bus_handle_t como pvParameters,
 * igual que antes. */
void ds3231_test(void *pvParameters);


#endif
