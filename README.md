# ESP32

## Proyecto educativo

Agarre todas las cositas que tenia en casa e intento armar un unico proyecto que
involucre mucho.

- Sensores de sonido (HC-SR04):
- Reloj Tiempo Real (RTC DS3231):
-
## build

Setup unica vez:
```
git clone --recursive https://github.com/espressif/esp-idf
cd esp-idf
./install.sh
source export.sh
```


Crear proyecto por primera vez:
```
cd ~
idf.py create-project env_controller_esp32
cd env_controller_esp32
idf.py set-target esp32
idf.py build
```

Al clonar este repo hay que repetir los primeros 3 pasos.

Luego cada vez que queremos trabajar:

```
cd env_controller_esp32
source ~/esp-idf/export.sh
idf.py build
```


Flashear firmware

```
idf.py -p /dev/ttyUSB0 flash
```

u otro puerto que se encuentre nuestro dispositivo.

Monitor serial:

```
idf.py monitor
```


##
