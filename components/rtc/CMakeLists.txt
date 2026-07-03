idf_component_register(
    SRCS "rtc.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio ds3231 freertos config esp_driver_i2c
)
