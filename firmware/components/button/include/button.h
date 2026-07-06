#ifndef BOTTON_H
#define BOTTON_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Inicializa el GPIO del boton y su interrupcion.
// Devuelve la queue donde se publican los eventos de click.
QueueHandle_t button_init(void);
uint32_t button_get_isr_count(void);
#endif
