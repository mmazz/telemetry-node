#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"


i2c_master_bus_handle_t i2c_bus_get(void);

void i2c_scan(i2c_master_bus_handle_t bus);

#endif
