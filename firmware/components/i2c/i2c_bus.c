#include "i2c_bus.h"
#include "esp_check.h"
#include "config.h"
#include <assert.h>

static char* TAG = "MUTEX";

static i2c_master_bus_handle_t bus_handle = NULL;
static SemaphoreHandle_t i2c_bus_mutex = NULL;

i2c_master_bus_handle_t i2c_bus_get(void)
{
    if (bus_handle != NULL)
        return bus_handle;

    i2c_master_bus_config_t config =
    {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, // entender un poco mas esto
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(&config, &bus_handle)
    );
    i2c_bus_mutex = xSemaphoreCreateMutex();
    assert(i2c_bus_mutex != NULL);


    return bus_handle;
}

void i2c_scan(i2c_master_bus_handle_t bus)
{
    printf("Escaneando bus I2C...\n");

    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        i2c_device_config_t dev_cfg = {
            .device_address = addr,
            .scl_speed_hz = 100000,
        };

        i2c_master_dev_handle_t dev;

        if (i2c_master_bus_add_device(bus, &dev_cfg, &dev) == ESP_OK)
        {
            esp_err_t ret = i2c_master_probe(bus, addr, 100);

            if (ret == ESP_OK)
            {
                printf("Encontrado dispositivo en 0x%02X\n", addr);
            }

            i2c_master_bus_rm_device(dev);
        }
    }

    printf("Fin del escaneo\n");
}
