idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES led button rtc lcd i2c imu radar uart_link telemetry
)
