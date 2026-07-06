idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES led radar button rtc lcd i2c imu
)
