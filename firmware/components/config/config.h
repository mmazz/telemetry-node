#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22

#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_16

#define BLINK_LED GPIO_NUM_15
#define ON_LED GPIO_NUM_25

#define TRIG_PIN GPIO_NUM_18
#define ECHO_PIN GPIO_NUM_19

#define BUTTON_PIN GPIO_NUM_4
#define I2C_PORT    0

#define LCD_ADDR 0x27
#define MPU6050_ADDR 0x69
#define RELOJ_ADDR 0x68 // La biblioteca ya la conoce asi que ni la usamos explicitamente

#define LCD_COLS 16
#define LCD_ROWS 2
// Tiempo minimo entre clicks validos, para filtrar el rebote mecanico
#define DEBOUNCE_US 150000  // 150 ms

#endif // !CONFIG_H
