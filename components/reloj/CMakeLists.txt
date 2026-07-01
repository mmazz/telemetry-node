idf_component_register(
    SRCS "reloj.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio ds3231 freertos config
)
