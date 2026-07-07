#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "radar.h"
#include "button.h"
#include "config.h"
#include "rtc.h"
#include "i2c_bus.h"
#include "lcd.h"
#include "imu.h"
/*
 *  Quiero tener 4 tareas:
 *      1. Sensores. Todos pueden muestrar cada 100ms maso
 *      2. LCD: actualizar la lcd que para el ojo humano deberia ser mayor a 500ms
 *      3. Enviar la info por UART (para luego ser por wifi)
 *      4. Boton de interrupcion.
 */


static const char *TAG = "main";

typedef struct
{
    bool radar_running;
    float distance_cm;
} lcd_msg_t;

static QueueHandle_t lcd_queue;

static volatile bool radar_running = true;

static void lcd_task(void *pvParameters)
{
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)pvParameters;

    if (lcd_init(bus) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando LCD");
        vTaskDelete(NULL);
        return;
    }

    lcd_clear();

    lcd_msg_t msg =
    {
        .radar_running = true,
        .distance_cm = -1
    };

    char temp[17];
    char line1[17];
    char line2[17];

    while (1)
    {
        /* Espera hasta que llegue un nuevo estado */
        if (xQueueReceive(lcd_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            snprintf(temp, sizeof(temp), "Telemetry: %s",
                     msg.radar_running ? "ON" : "OFF");
            snprintf(line1, sizeof(line1), "%-16s", temp);

            if (msg.distance_cm >= 0)
            {
                snprintf(temp, sizeof(temp), "Dist:%6.1f cm", msg.distance_cm);
            }
            else
            {
                snprintf(temp, sizeof(temp), "Sin lectura");
            }
            snprintf(line2, sizeof(line2), "%-16s", temp);

            lcd_set_cursor(0, 0);
            lcd_print(line1);

            lcd_set_cursor(0, 1);
            lcd_print(line2);
        }
    }
}

static void radar_task(void *pvParameters)
{
    lcd_msg_t lcd_msg =
    {
        .radar_running = true,
        .distance_cm = -1
    };

    while (1)
    {
        if (radar_running)
        {
            float d = radar_get_last_distance_cm();

            lcd_msg.radar_running = true;
            lcd_msg.distance_cm = d;

            xQueueOverwrite(lcd_queue, &lcd_msg);

            if (d >= 0)
            {
                ESP_LOGI(TAG, "Distancia: %.2f cm", d);
            }
            else
            {
                ESP_LOGI(TAG, "Sin lectura valida");
            }
        }
        else
        {
            /* Si el radar está detenido también avisamos al LCD */
            lcd_msg.radar_running = false;

            xQueueOverwrite(lcd_queue, &lcd_msg);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}



typedef struct
{
    rtc_dev_t rtc;
    radar_dev_t radar;
    imu_dev_t imu;
} telemetry_node_t;
static telemetry_node_t node;

void telemetry_node_init()
{
    i2c_master_bus_handle_t bus = i2c_bus_get();
    ESP_LOGI(TAG, "Starting telemetry node");
    gpio_set_direction(ON_LED, GPIO_MODE_OUTPUT);

    rtc_ds_init(bus, &node.rtc);
    radar_init(&node.radar);
    imu_init(bus, &node.imu);
}

void sensor_task(void *pvParameters)
{
    telemetry_node_t *node = (telemetry_node_t *)pvParameters;

    float distance_cm;
    struct tm time_rtc;

    while (1)
    {
        esp_err_t dist_ok = radar_get_distance(&node->radar, &distance_cm);

        esp_err_t rtc_ok = rtc_get_time(&node->rtc, &time_rtc);
        imu_data_t imu_data;

        esp_err_t imu__ok = imu_read(&node->imu, &imu_data);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void app_main(void)
{

    telemetry_node_init();
    i2c_master_bus_handle_t bus = i2c_bus_get();


    lcd_queue = xQueueCreate(1, sizeof(lcd_msg_t));
    if (lcd_queue == NULL)
    {
        ESP_LOGE(TAG, "No se pudo crear lcd_queue");
        return;
    }

    i2c_scan(bus);

    BaseType_t ok;
    ok = xTaskCreate(mpu6050_test, "mpu6050_test", configMINIMAL_STACK_SIZE * 3,
                      bus, 5, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear mpu6050_test");

    ok = xTaskCreate(ds3231_test, "ds3231_test", configMINIMAL_STACK_SIZE * 3,
                    bus, 5, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear ds3231_test");

    ok = xTaskCreate(radar_task, "radar_task", 4096, NULL, 5, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear radar_task");

    ok = xTaskCreate(lcd_task, "lcd_task", 4096, bus, 5, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear lcd_task");

    lcd_msg_t lcd_msg =
    {
        .radar_running = true,
        .distance_cm = -1
    };
    xQueueOverwrite(lcd_queue, &lcd_msg);

    QueueHandle_t button_queue = button_init();
    uint8_t dummy;
    while (1)
    {
        bool toggle_requested = false;

        /* Espera bloqueante hasta 200 ms por un evento de botón,
         * en vez de hacer polling activo con timeout 0. */
        if (xQueueReceive(button_queue, &dummy, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            toggle_requested = true;

            /* Drena eventos extra (rebotes / clicks acumulados)
             * sin bloquear, para togglear una sola vez. */
            while (xQueueReceive(button_queue, &dummy, 0) == pdTRUE)
            {
            }
        }

        if (toggle_requested)
        {
            radar_running = !radar_running;
            gpio_set_level(ON_LED, radar_running);
            radar_set_paused(!radar_running);
            ESP_LOGI(TAG, "%s",
                     radar_running ? "Radar reanudado"
                                   : "Radar pausado");
        }
    }
}
