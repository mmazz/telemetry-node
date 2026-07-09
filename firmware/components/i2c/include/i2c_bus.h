#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct
{
    i2c_master_bus_handle_t handle;
} i2c_bus_t;

i2c_master_bus_handle_t i2c_bus_get(void);

void i2c_scan(i2c_master_bus_handle_t bus);
extern SemaphoreHandle_t i2c_mutex;
#endif
