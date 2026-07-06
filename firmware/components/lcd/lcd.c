#include "lcd.h"
#include "config.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define LCD_RS          (1 << 0)
#define LCD_RW          (1 << 1)
#define LCD_EN          (1 << 2)
#define LCD_BACKLIGHT   (1 << 3)

#define LCD_D4          (1 << 4)
#define LCD_D5          (1 << 5)
#define LCD_D6          (1 << 6)
#define LCD_D7          (1 << 7)

static uint8_t lcd_control = LCD_BACKLIGHT;
static const char *TAG = "LCD";
static i2c_master_dev_handle_t lcd_dev = NULL;



static esp_err_t lcd_write(uint8_t value)
{
    return i2c_master_transmit(
        lcd_dev,
        &value,
        1,
        -1);
}
static esp_err_t lcd_pulse_enable(uint8_t data)
{
    ESP_RETURN_ON_ERROR(
        lcd_write(data | LCD_EN),
        TAG,
        "Error EN high"
    );

    esp_rom_delay_us(1);

    ESP_RETURN_ON_ERROR(
        lcd_write(data & ~LCD_EN),
        TAG,
        "Error EN low"
    );

    esp_rom_delay_us(50);

    return ESP_OK;
}

static esp_err_t lcd_send_nibble(uint8_t nibble, bool rs)
{
    uint8_t data = lcd_control;

    if (rs)
    {
        data |= LCD_RS;
    }

    if (nibble & 0x01)
        data |= LCD_D4;

    if (nibble & 0x02)
        data |= LCD_D5;

    if (nibble & 0x04)
        data |= LCD_D6;

    if (nibble & 0x08)
        data |= LCD_D7;

    return lcd_pulse_enable(data);
}
static esp_err_t lcd_send_byte(uint8_t value, bool rs)
{
    esp_err_t ret;

    ret = lcd_send_nibble(value >> 4, rs);
    if (ret != ESP_OK)
        return ret;

    ret = lcd_send_nibble(value & 0x0F, rs);
    if (ret != ESP_OK)
        return ret;

    return ESP_OK;
}


static esp_err_t lcd_send_command(uint8_t cmd)
{
    esp_err_t ret = lcd_send_byte(cmd, false);
    if (cmd == 0x01 || cmd == 0x02) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return ret;
}

esp_err_t lcd_init(i2c_master_bus_handle_t bus)
{
    i2c_device_config_t config =
    {
        .device_address = LCD_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(bus, &config, &lcd_dev),
        TAG, "Error agregando LCD"
    );
    vTaskDelay(pdMS_TO_TICKS(3000)); // 3 segundos para que puedas mirar el LCD
    // Esperar a que el LCD termine su power-on reset (datasheet pide >40ms)
    vTaskDelay(pdMS_TO_TICKS(50));

    // Forzar modo 8 bits -> 4 bits (secuencia clásica del datasheet)
    lcd_send_nibble(0x03, false);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x03, false);
    esp_rom_delay_us(150);
    lcd_send_nibble(0x03, false);
    esp_rom_delay_us(150);
    lcd_send_nibble(0x02, false); // ahora sí, modo 4 bits

    // A partir de acá ya se puede usar lcd_send_command (envía 2 nibbles)
    lcd_send_command(0x28); // Function set: 4 bits, 2 líneas, fuente 5x8
    lcd_send_command(0x08); // Display off
    lcd_send_command(0x01); // Clear display
    vTaskDelay(pdMS_TO_TICKS(2)); // Clear tarda ~1.6ms, no 50us
    lcd_send_command(0x06); // Entry mode: incrementa, sin shift
    lcd_send_command(0x0C); // Display on, cursor off, blink off

    return ESP_OK;
}

static esp_err_t lcd_send_data(uint8_t data)
{
    return lcd_send_byte(data, true);
}

esp_err_t lcd_clear(void)
{
    esp_err_t ret = lcd_send_command(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    return ret;
}

esp_err_t lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    return lcd_send_command(0x80 | (col + row_offsets[row]));
}

esp_err_t lcd_print(const char *str)
{
    while (*str) {
        esp_err_t ret = lcd_send_data((uint8_t)*str++);
        if (ret != ESP_OK) return ret;
    }
    return ESP_OK;
}
