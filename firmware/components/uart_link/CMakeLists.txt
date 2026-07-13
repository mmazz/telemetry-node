idf_component_register(
    SRCS "uart_link.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_timer freertos config
)
