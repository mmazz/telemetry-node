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
 *
 *  No requiero mutex en los sensores i2c ya que hay una unica task que los usa.
 */

static const char *TAG = "main";

typedef struct
{
    rtc_dev_t rtc;
    imu_dev_t imu;
    radar_dev_t radar;
    i2c_master_bus_handle_t bus;
} telemetry_node_t;

typedef struct
{
    float distance_cm;
    struct tm time;
    imu_data_t imu;
} telemetry_data_t;


static telemetry_node_t node;
static QueueHandle_t sensors_data_queue;
typedef struct
{
    bool sensors_running;
    float distance_cm;
} lcd_msg_t;

static QueueHandle_t lcd_queue;

static volatile bool sensors_running = true;

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
        .sensors_running = true,
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
                     msg.sensors_running ? "ON" : "OFF");
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


void telemetry_node_init()
{
    ESP_LOGI(TAG, "Starting telemetry node");
    //registrar manejadores de interrupción para pines individuales. Ejemplo el boton.
    gpio_install_isr_service(0);
    gpio_set_direction(ON_LED, GPIO_MODE_OUTPUT);

    i2c_master_bus_handle_t bus = i2c_bus_get();

    ESP_LOGI(TAG, "Starting data queue");
    sensors_data_queue = xQueueCreate(5, sizeof(telemetry_data_t));
    rtc_ds_init(bus, &node.rtc);
    imu_init(bus, &node.imu);
    radar_init(&node.radar);
    ESP_LOGI(TAG, "Finish Init");
}

void sensor_task(void *pvParameters)
{
    telemetry_node_t *node = (telemetry_node_t *)pvParameters;

    static telemetry_data_t data;
    lcd_msg_t lcd_msg =
    {
        .sensors_running = true,
        .distance_cm = -1
    };

    while (1)
    {
        if(sensors_running)
        {
            esp_err_t dist_ok = radar_get_distance(&node->radar, &data.distance_cm);
            esp_err_t rtc_ok = rtc_get_time(&node->rtc, &data.time);
            esp_err_t imu_ok = imu_read(&node->imu, &data.imu);
            if (dist_ok == ESP_OK && rtc_ok  == ESP_OK && imu_ok  == ESP_OK)
            {
                xQueueSend(sensors_data_queue, &data, pdMS_TO_TICKS(60));
            }
            else
                ESP_LOGE(TAG, "No se pudieron realizar todas las mediciones de forma correcta");
            if (dist_ok == ESP_OK )
            {
                lcd_msg.sensors_running = true;
                lcd_msg.distance_cm = data.distance_cm;
                xQueueOverwrite(lcd_queue, &lcd_msg);
            }
        }
        else
        {
            lcd_msg.sensors_running = false;
            lcd_msg.distance_cm = -1;
            xQueueOverwrite(lcd_queue, &lcd_msg);

        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{

    telemetry_node_init();
    i2c_master_bus_handle_t bus = i2c_bus_get();
    i2c_scan(bus);

    lcd_queue = xQueueCreate(1, sizeof(lcd_msg_t));
    if (lcd_queue == NULL)
    {
        ESP_LOGE(TAG, "No se pudo crear lcd_queue");
        return;
    }

    node.bus = bus;
    BaseType_t ok;

    ok = xTaskCreate(lcd_task, "lcd_task", 4096, bus, 4, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear el task del lcd");
    ok = xTaskCreate(sensor_task, "sensor_task", configMINIMAL_STACK_SIZE * 3,
                      &node, 5, NULL);
    if (ok != pdPASS)
        ESP_LOGE(TAG, "No se pudo crear el task de senor");


    lcd_msg_t lcd_msg =
    {
        .sensors_running = true,
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
            sensors_running = !sensors_running;
            gpio_set_level(ON_LED, sensors_running);
            ESP_LOGI(TAG, "%s", sensors_running ? "Sensores reanudados" : "Sensores pausados");
        }
    }
}



//void send_data(void *pvParameters)
//{
//    telemetry_node_t *node = (telemetry_node_t *)pvParameters;
//    if (xQueueReceive(sensors_queue, &data, portMAX_DELAY) == pdTRUE)
//    {
//
//    }
//}


