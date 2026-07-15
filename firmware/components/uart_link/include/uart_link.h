#ifndef UART_H
#define UART_H

#include "telemetry.h"

void uart_init(void);

void uart_send(const char* msg);
void uart_send_telemetry(const telemetry_data_t *data);


#endif // !UART_H
