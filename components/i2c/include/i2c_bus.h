#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"

i2c_master_bus_handle_t i2c_bus_init(void);


// Como usarlo?
//
// i2c_master_bus_handle_t bus2 = i2c_bus_init();
// i2c_scan(bus2);
// lcd_init(bus2);
void i2c_scan(i2c_master_bus_handle_t bus);

#endif
