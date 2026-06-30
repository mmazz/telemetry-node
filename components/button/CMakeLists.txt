idf_component_register(
    SRCS "button.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_timer freertos config
)
