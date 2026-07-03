#ifndef LCD_H
#define LCD_H
#include "driver/i2c_master.h"

esp_err_t lcd_init(i2c_master_bus_handle_t bus);
esp_err_t lcd_clear(void);
esp_err_t lcd_set_cursor(uint8_t col, uint8_t row);
esp_err_t lcd_print(const char *str);

#endif // !LCD_H
