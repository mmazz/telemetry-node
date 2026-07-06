#include "imu.h"
#include "config.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "imu";

#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B  /* accel(6) + temp(2) + gyro(6) = 14 bytes */

#define ACCEL_SENS_2G  16384.0f   /* LSB por g, con ACCEL_CONFIG = 0x00 (+-2g) */
#define GYRO_SENS_250  131.0f     /* LSB por dps, con GYRO_CONFIG = 0x00 (+-250dps) */

static inline int16_t raw16(const uint8_t *buf)
{
    return (int16_t)((buf[0] << 8) | buf[1]);
}

esp_err_t imu_init(i2c_master_bus_handle_t bus, imu_dev_t *dev)
{
    i2c_device_config_t cfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(bus, &cfg, &dev->handle);
    if (err != ESP_OK)
    {
        return err;
    }

    /* El MPU6050 arranca en sleep mode, hay que despertarlo escribiendo 0
       en PWR_MGMT_1 (tambien selecciona el clock interno) */
    uint8_t wake[2] = { MPU6050_REG_PWR_MGMT_1, 0x00 };
    err = i2c_master_transmit(dev->handle, wake, sizeof(wake), pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }

    /* Rango del acelerometro +-2g (default, queda explicito) */
    uint8_t accel_cfg[2] = { MPU6050_REG_ACCEL_CONFIG, 0x00 };
    err = i2c_master_transmit(dev->handle, accel_cfg, sizeof(accel_cfg), pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        return err;
    }

    /* Rango del giroscopio +-250 dps (default, queda explicito) */
    uint8_t gyro_cfg[2] = { MPU6050_REG_GYRO_CONFIG, 0x00 };
    return i2c_master_transmit(dev->handle, gyro_cfg, sizeof(gyro_cfg), pdMS_TO_TICKS(100));
}

static esp_err_t imu_read_all(imu_dev_t *dev, uint8_t *buf14)
{
    uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;

    return i2c_master_transmit_receive(dev->handle,
                                        &reg, 1,
                                        buf14, 14,
                                        pdMS_TO_TICKS(100));
}

esp_err_t imu_get_accel(imu_dev_t *dev, float *ax, float *ay, float *az)
{
    uint8_t buf[14];

    esp_err_t err = imu_read_all(dev, buf);
    if (err != ESP_OK)
    {
        return err;
    }

    *ax = raw16(&buf[0]) / ACCEL_SENS_2G;
    *ay = raw16(&buf[2]) / ACCEL_SENS_2G;
    *az = raw16(&buf[4]) / ACCEL_SENS_2G;

    return ESP_OK;
}

esp_err_t imu_get_gyro(imu_dev_t *dev, float *gx, float *gy, float *gz)
{
    uint8_t buf[14];

    esp_err_t err = imu_read_all(dev, buf);
    if (err != ESP_OK)
    {
        return err;
    }

    *gx = raw16(&buf[8])  / GYRO_SENS_250;
    *gy = raw16(&buf[10]) / GYRO_SENS_250;
    *gz = raw16(&buf[12]) / GYRO_SENS_250;

    return ESP_OK;
}

esp_err_t imu_get_temp(imu_dev_t *dev, float *temp_c)
{
    uint8_t buf[14];

    esp_err_t err = imu_read_all(dev, buf);
    if (err != ESP_OK)
    {
        return err;
    }

    int16_t raw = raw16(&buf[6]);
    *temp_c = (raw / 340.0f) + 36.53f;

    return ESP_OK;
}

void mpu6050_test(void *pvParameters)
{
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)pvParameters;
    imu_dev_t dev;

    ESP_ERROR_CHECK(imu_init(bus, &dev));

    while (1)
    {
        float ax, ay, az, gx, gy, gz, temp_c;

        vTaskDelay(pdMS_TO_TICKS(250));

        if (imu_get_accel(&dev, &ax, &ay, &az) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo leer el acelerometro");
            continue;
        }

        if (imu_get_gyro(&dev, &gx, &gy, &gz) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo leer el giroscopio");
            continue;
        }

        if (imu_get_temp(&dev, &temp_c) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo leer la temperatura");
            continue;
        }

        ESP_LOGI(TAG, "accel[g] x=%.2f y=%.2f z=%.2f | gyro[dps] x=%.2f y=%.2f z=%.2f | %.2f degC",
                 ax, ay, az, gx, gy, gz, temp_c);
    }
}

