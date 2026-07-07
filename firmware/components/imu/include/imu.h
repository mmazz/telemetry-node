#ifndef IMU_H
#define IMU_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// Poner el A0 en Vin asi el registro es 0x69 y no el 0x68 que es el de default del rtc
typedef struct
{
    i2c_master_dev_handle_t handle;
} imu_dev_t;

typedef struct
{
    float x;
    float y;
    float z;
} imu_vec3_t;

typedef struct
{
    imu_vec3_t accel;
    imu_vec3_t gyro;
} imu_data_t;

esp_err_t imu_init(i2c_master_bus_handle_t bus, imu_dev_t *dev);

/* Aceleración en g (1g = 9.81 m/s^2), rango por defecto +-2g */
esp_err_t imu_get_accel(imu_dev_t *dev, imu_vec3_t *accel);

/* Velocidad angular en grados/seg, rango por defecto +-250 dps */
esp_err_t imu_get_gyro(imu_dev_t *dev, imu_vec3_t *gyro);

esp_err_t imu_read(imu_dev_t *dev, imu_data_t *data);

/* Temperatura interna del MPU6050 en grados C */
esp_err_t imu_get_temp(imu_dev_t *dev, float *temp_c);

void mpu6050_test(void *pvParameters);

#endif // !IMU_H
