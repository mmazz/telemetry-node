# ESP32 Telemetry Node

## Descripción

Proyecto personal de exploración sobre sistemas embebidos utilizando ESP32 y FreeRTOS.

El objetivo es desarrollar un nodo de adquisición de datos que integre distintos periféricos y transmita la información a una Raspberry Pi para su procesamiento y visualización.

Actualmente integra los siguientes módulos:

- HC-SR04 (sensor ultrasónico de distancia)
- DS3231 (RTC)
- LCD 16x2 (LMB162HBC)
- PCF8574 (adaptador I2C para LCD)
- MPU 6050 (Acelerometro y giroscopio)

La Raspberry Pi actúa como estación receptora, mostrando y registrando la telemetría enviada por el nodo.

## Objetivos técnicos

- Programación en C utilizando ESP-IDF.
- Organización del software mediante FreeRTOS.
- Integración de múltiples periféricos.
- Comunicación entre sistemas embebidos y Linux.
- Experimentación con distintos mecanismos de comunicación (UART / WiFi).

## Estado actual

Funcionando sin wifi.


## Requisitos

- ESP-IDF
- Git
- Python (según los requisitos de ESP-IDF)

## Instalación de ESP-IDF

Solo es necesario una vez.

```bash
git clone --recursive https://github.com/espressif/esp-idf
cd esp-idf
./install.sh
```

## Clonar el proyecto

```bash
git clone https://github.com/mmazz/telemetry-node
cd telemetry-node/firmware
```

## Configurar el entorno

Debe ejecutarse al abrir una nueva terminal.

```bash
source ~/esp-idf/export.sh
```

## Dependencias

```bash
idf.py add-dependency esp-idf-lib/ds3231
```

## Compilar

```bash
idf.py build
```

## Flashear

```bash
idf.py -p /dev/ttyUSB0 flash
```

## Monitor serie

```bash
idf.py -p /dev/ttyUSB0 monitor
```

