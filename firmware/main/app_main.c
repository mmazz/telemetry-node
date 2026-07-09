#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "radar.h"
#include "button.h"
#include "config.h"
#include "rtc.h"
#include "i2c_bus.h"
#include "lcd.h"
#include "imu.h"
#include <stdatomic.h>

/*
 *  Quiero tener 4 tareas:
 *      1. Sensores. Todos pueden muestrar cada 100ms maso
 *      2. LCD: actualizar la lcd que para el ojo humano deberia ser mayor a 500ms
 *      3. Enviar la info por UART (para luego ser por wifi)
 *      4. Boton de interrupcion.
 *
 *  No requiero mutex en los sensores i2c ya que hay una unica task que los usa.
 */

static const char *TAG = "main";

static i2c_master_bus_handle_t s_i2c_bus; // módulo de infraestructura, no de telemetría

typedef struct
{
    rtc_dev_t rtc;
    imu_dev_t imu;
    radar_dev_t radar;
} telemetry_node_t;


typedef struct
{
    float distance_cm;
    bool  distance_valid;
    struct tm time;
    bool  time_valid;
    imu_data_t imu;
    bool  imu_valid;
    int64_t timestamp_us;   // esp_timer_get_time(), para detectar datos viejos
} telemetry_data_t;

static QueueHandle_t lcd_queue;   // longitud 1, overwrite -> "último valor"
static QueueHandle_t uart_queue;  // longitud N, FIFO -> no perder tramas

static telemetry_node_t node;

static void telemetry_publish(const telemetry_data_t *data)
{
    xQueueOverwrite(lcd_queue, data);          // LCD: siempre el último dato
    xQueueSend(uart_queue, data, 0);           // UART: FIFO, no bloqueante
}
static atomic_bool sensors_running = true;

void telemetry_node_init(void)
{
    ESP_LOGI(TAG, "Starting telemetry node");
    s_i2c_bus = i2c_bus_get();
    //registrar manejadores de interrupción para pines individuales. Ejemplo el boton.
    gpio_install_isr_service(0);
    gpio_set_direction(ON_LED, GPIO_MODE_OUTPUT);
    i2c_mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Starting data queue");
    uart_queue = xQueueCreate(5, sizeof(telemetry_data_t));
    lcd_queue = xQueueCreate(1, sizeof(telemetry_data_t));
    rtc_ds_init(s_i2c_bus, &node.rtc);
    imu_init(s_i2c_bus, &node.imu);
    radar_init(&node.radar);
    ESP_LOGI(TAG, "Finish Init");
}

i2c_master_bus_handle_t telemetry_get_bus(void) {
    return s_i2c_bus;
}


static void lcd_task(void *pvParameters)
{
    i2c_master_bus_handle_t bus = (i2c_master_bus_handle_t)pvParameters;
    if (lcd_init(bus) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando LCD");
        vTaskDelete(NULL);
        return;
    }
    lcd_clear(); // una sola vez, al inicio

    telemetry_data_t msg = { .distance_cm = -1, .distance_valid = false };
    char temp[17];
    char line1[17];
    char line2[17];

    while (1)
    {
        // No importa si llega dato nuevo o no: msg conserva el último valor conocido.
        xQueueReceive(lcd_queue, &msg, pdMS_TO_TICKS(200));

        bool running = atomic_load(&sensors_running);

        if (!running)
        {
            snprintf(line1, sizeof(line1), "%-16s", "** PAUSADO **");
            snprintf(line2, sizeof(line2), "%-16s", "");
        }
        else
        {
            snprintf(temp, sizeof(temp), "Telemetry: ON");
            snprintf(line1, sizeof(line1), "%-16s", temp);

            if (msg.distance_valid)
                snprintf(temp, sizeof(temp), "Dist:%6.1f cm", msg.distance_cm);
            else
                snprintf(temp, sizeof(temp), "Sin lectura");
            snprintf(line2, sizeof(line2), "%-16s", temp);
        }

        // El redibujado corre SIEMPRE, no solo cuando llega dato nuevo.
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
        lcd_set_cursor(0, 0);
        lcd_print(line1);
        lcd_set_cursor(0, 1);
        lcd_print(line2);
        xSemaphoreGive(i2c_mutex);
    }
}


static void sensor_task(void *pvParameters)
{
    telemetry_node_t *node = (telemetry_node_t *)pvParameters;

    static telemetry_data_t data;

    while (1)
    {
        if (atomic_load(&sensors_running))
        {
            data.distance_valid = (radar_get_distance(&node->radar, &data.distance_cm) == ESP_OK);
            xSemaphoreTake(i2c_mutex, portMAX_DELAY);
            data.time_valid     = (rtc_get_time(&node->rtc, &data.time) == ESP_OK);
            data.imu_valid      = (imu_read(&node->imu, &data.imu) == ESP_OK);
            xSemaphoreGive(i2c_mutex);
            data.timestamp_us   = esp_timer_get_time();

            telemetry_publish(&data);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void uart_task(void *pvParameters)
{
    telemetry_data_t msg;
    while (1)
    {
        if (xQueueReceive(uart_queue, &msg, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            if (atomic_load(&sensors_running))
            {
                // serializar y transmitir msg
            }
            // si está pausado, simplemente no se envía (ya no llegan datos nuevos igual,
            // porque sensor_task no publica mientras sensors_running == false)
        }
    }
}

static void button_task(void *pvParameters)
{
    QueueHandle_t q = (QueueHandle_t)pvParameters;
    uint8_t dummy;
    while (1)
    {
        if (xQueueReceive(q, &dummy, portMAX_DELAY) == pdTRUE)
        {
            bool current = atomic_load(&sensors_running);
            atomic_store(&sensors_running, !current);
            gpio_set_level(ON_LED, !current);
            ESP_LOGI(TAG, "Sensores %s", !current ? "reanudados" : "pausados");
        }
    }
}

void app_main(void)
{

    telemetry_node_init();
    i2c_master_bus_handle_t bus = i2c_bus_get();
    i2c_scan(bus);

    BaseType_t ok;

    ok = xTaskCreate(lcd_task, "lcd_task", 4096,  telemetry_get_bus(), 4, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear el task del lcd");

    ok = xTaskCreate(sensor_task, "sensor_task", 4096, &node, 5, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear el task de senor");

    QueueHandle_t btn_queue = button_init();
    ok = xTaskCreate(button_task, "button_task", 2048, btn_queue, 6, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear el task de senor");

}



//void send_data(void *pvParameters)
//{
//    telemetry_node_t *node = (telemetry_node_t *)pvParameters;
//    if (xQueueReceive(sensors_queue, &data, portMAX_DELAY) == pdTRUE)
//    {
//
//    }
//}


