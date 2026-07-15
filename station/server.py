"""
Servidor UART para Raspberry Pi 3.
Recibe un paquete CSV enviado por el ESP32 (una linea por muestra),
lo parsea y loguea. Si no llega data en TIMEOUT_SEC segundos, loguea
un warning.

Formato de linea enviado por el ESP32 (ver snprintf del lado firmware):

    timestamp_us,distance_valid,distance_cm,HH:MM:SS,imu_valid,
    accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z\n

Requiere: pip install pyserial
"""

import serial
import logging
import threading
import time
import sys
import datetime
from dataclasses import dataclass
from typing import Optional, Iterator

# ---------------------------------------------------------------------------
# CONFIGURACION
# ---------------------------------------------------------------------------

SERIAL_PORT = '/dev/serial0'   # UART de los pines GPIO14/15 (con BT deshabilitado).
                                # Si usas un conversor USB-serie, usa '/dev/ttyUSB0'.
BAUD_RATE = 115200
TIMEOUT_SEC = 5                # segundos sin datos antes de loguear warning
RECONNECT_WAIT_SEC = 2         # espera antes de reintentar abrir el puerto tras un error

NUM_FIELDS = 11                # cantidad de campos separados por coma esperados

LOG_EVERY_N = 20               # loguear a INFO 1 de cada N muestras (resto va a DEBUG)

# ---------------------------------------------------------------------------
# LOGGING
# ---------------------------------------------------------------------------

logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.FileHandler('esp32_server.log'),
        logging.StreamHandler(sys.stdout),
    ],
)
logger = logging.getLogger("esp32_server")
# La consola solo muestra INFO+ para no inundar; el archivo se queda con DEBUG.
for h in logger.handlers:
    pass
logging.getLogger().handlers[1].setLevel(logging.INFO)  # StreamHandler(stdout)

# ---------------------------------------------------------------------------
# MODELO DE DATO
# ---------------------------------------------------------------------------

@dataclass
class SensorSample:
    timestamp_us: int
    distance_valid: bool
    distance_cm: float
    time_str: str          # HH:MM:SS tal cual llega del ESP32
    imu_valid: bool
    accel_x: float
    accel_y: float
    accel_z: float
    gyro_x: float
    gyro_y: float
    gyro_z: float
    received_at: datetime.datetime  # timestamp del lado Raspberry (fecha la pone el ground station)


def parse_line(line: str) -> Optional[SensorSample]:
    """Parsea una linea CSV. Devuelve None si esta mal formada (se descarta)."""
    fields = line.strip().split(',')
    if len(fields) != NUM_FIELDS:
        logger.warning(f"Linea con {len(fields)} campos (esperaba {NUM_FIELDS}), descartada: {line!r}")
        return None

    try:
        return SensorSample(
            timestamp_us=int(fields[0]),
            distance_valid=bool(int(fields[1])),
            distance_cm=float(fields[2]),
            time_str=fields[3],
            imu_valid=bool(int(fields[4])),
            accel_x=float(fields[5]),
            accel_y=float(fields[6]),
            accel_z=float(fields[7]),
            gyro_x=float(fields[8]),
            gyro_y=float(fields[9]),
            gyro_z=float(fields[10]),
            received_at=datetime.datetime.now(),
        )
    except ValueError as e:
        logger.warning(f"Error parseando campos ({e}), linea descartada: {line!r}")
        return None


_sample_count = 0


def log_sample(s: SensorSample):
    """Loguea la muestra. Para no inundar el log a alta frecuencia, solo
    1 de cada LOG_EVERY_N muestras sube a INFO; el resto queda en DEBUG
    (sigue quedando en el archivo, pero no en consola)."""
    global _sample_count
    _sample_count += 1

    dist = f"{s.distance_cm:.1f}cm" if s.distance_valid else "invalida"
    imu = (
        f"acc=({s.accel_x:.2f},{s.accel_y:.2f},{s.accel_z:.2f}) "
        f"gyro=({s.gyro_x:.2f},{s.gyro_y:.2f},{s.gyro_z:.2f})"
        if s.imu_valid else "invalida"
    )
    msg = f"Dato recibido -> t_us={s.timestamp_us} hora={s.time_str} dist={dist} imu={imu}"

    if _sample_count % LOG_EVERY_N == 0:
        logger.info(msg)
    else:
        logger.debug(msg)

# ---------------------------------------------------------------------------
# LECTURA DE LINEAS (desacoplada del transporte, para poder cambiar a WiFi despues)
# ---------------------------------------------------------------------------

def lines_from_serial(ser: serial.Serial) -> Iterator[Optional[str]]:
    """Generador que yield-ea una linea decodificada, o None si hubo timeout
    de lectura (sin dato disponible). Puede propagar serial.SerialException
    si se pierde la conexion (por ejemplo, se desconecta el cable)."""
    while True:
        raw = ser.readline()  # respeta ser.timeout
        if not raw:
            yield None
            continue
        try:
            yield raw.decode('utf-8', errors='replace')
        except Exception as e:
            logger.warning(f"Error decodificando linea: {e}")
            yield None

# ---------------------------------------------------------------------------
# WATCHDOG
# ---------------------------------------------------------------------------

last_data_time = time.monotonic()
lock = threading.Lock()


def watchdog():
    already_warned = False
    while True:
        time.sleep(1)
        with lock:
            elapsed = time.monotonic() - last_data_time
        if elapsed > TIMEOUT_SEC:
            if not already_warned:
                logger.warning(f"Sin datos hace {elapsed:.1f}s (umbral: {TIMEOUT_SEC}s)")
                already_warned = True
        else:
            already_warned = False

# ---------------------------------------------------------------------------
# MAIN
# ---------------------------------------------------------------------------

def run_once() -> bool:
    """Abre el puerto, lee hasta que se corte la conexion o el usuario
    interrumpa. Devuelve True si hay que reintentar, False si hay que salir
    del todo (Ctrl+C o error de apertura del puerto)."""
    global last_data_time

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        logger.error(f"No se pudo abrir el puerto serie: {e}")
        return False

    logger.info("Esperando datos...")

    try:
        for line in lines_from_serial(ser):
            if line is None:
                continue
            sample = parse_line(line)
            if sample is not None:
                with lock:
                    last_data_time = time.monotonic()
                log_sample(sample)
    except serial.SerialException as e:
        logger.error(f"Se perdio la conexion serie: {e}. Reintentando en {RECONNECT_WAIT_SEC}s...")
        return True
    except KeyboardInterrupt:
        logger.info("Cerrando servidor (Ctrl+C)")
        return False
    finally:
        try:
            ser.close()
        except Exception:
            pass

    return True


def main():
    logger.info(f"Iniciando servidor UART en {SERIAL_PORT} @ {BAUD_RATE} baudios")

    threading.Thread(target=watchdog, daemon=True).start()

    try:
        while True:
            should_retry = run_once()
            if not should_retry:
                break
            time.sleep(RECONNECT_WAIT_SEC)
    except KeyboardInterrupt:
        logger.info("Cerrando servidor (Ctrl+C)")


if __name__ == '__main__':
    main()
