#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"

#define BLINK_LED GPIO_NUM_15
#define I2C_SDA GPIO_NUM_21
#define I2C_SDL GPIO_NUM_22
#define BLINK_LED GPIO_NUM_15
#define ON_LED GPIO_NUM_16
#define TRIG_PIN GPIO_NUM_18
#define ECHO_PIN GPIO_NUM_19
#define BUTTON_PIN GPIO_NUM_4

// Tiempo minimo entre clicks validos, para filtrar el rebote mecanico
#define DEBOUNCE_US 150000  // 150 ms

#endif // !CONFIG_H
