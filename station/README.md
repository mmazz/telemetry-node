# Station server

## Usage

En este caso particular, se esta usando una Raspberry pi 3.




### UART pines fisicos

A modo de testing una de las pruebas fue mediante uart con los pines fisicos de
la raspberry pi.
Para ello se requiere liberar estos pines que por defecto estan tomados por el
bluetooth.

Para ello:

```
sudo raspi-config
```

Y realizamos lo siguiente:
* Interface Options
    * → Serial Port
        * → "¿Login shell sobre serie?" No
        * → "¿Habilitar hardware serial?" Sí
        * → Reiniciar.


|    ESP32      | Raspberry Pi 3|
|---------------|---------------|
| TX | GPIO15 / RXD (pin físico 10)|
| RX | GPIO14 / TXD (pin físico 8)|
| GND| GND (pin físico 6, por ejemplo)|


### Dependencias

```
sudo apt install python3-pip -y
sudo apt install python3-serial
```

## Uso

```
python3 server.py
```

### Permisos

Si no deja abrir el puerto

```
sudo usermod -aG dialout $USER
# cerrar sesión y volver a entrar para que aplique
```
