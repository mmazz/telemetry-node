#include "uart_link.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "driver/uart.h"
const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_event_queue;
const static uart_port_t uart_num = UART_NUM_2;
void uart_init(){

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_event_queue, 0));

}


void uart_send(const char* msg)
{
    uart_write_bytes(uart_num, (const char*)msg, strlen(msg));
}

void uart_send_telemetry(const telemetry_data_t *data)
{
    char buf[128];  // tenés que calcular un tamaño que alcance para el peor caso

    int len = snprintf(buf, sizeof(buf),
        "%llu,%d,%.1f,%02d:%02d:%02d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        data->timestamp_us,
        data->distance_valid, data->distance_cm,
        data->time.tm_hour, data->time.tm_min, data->time.tm_sec,
        data->imu_valid, data->imu.accel.x, data->imu.accel.y, data->imu.accel.z,
        data->imu.gyro.x, data->imu.gyro.y, data->imu.gyro.z
    );

    if (len > 0 && len < sizeof(buf))
    {
        uart_send(buf);
    }
}
