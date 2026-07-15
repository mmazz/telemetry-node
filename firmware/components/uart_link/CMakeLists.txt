idf_component_register(
    SRCS "uart_link.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_driver_uart freertos config telemetry
)
