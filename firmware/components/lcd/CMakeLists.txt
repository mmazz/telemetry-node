idf_component_register(
    SRCS "lcd.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_timer freertos config esp_driver_i2c i2c
)
