# esp32-plc-gateway

Firmware para la ESP32-WROOM-32D que actua como PLC-Gateway dentro del honeypot IoT. Tiene dos roles: recibe eventos de la ESP32 camara por HTTP POST y los reenvía al servidor FastAPI, y expone sus propios endpoints simulando un controlador industrial que puede ser atacado directamente.

## Requisitos

- PlatformIO instalado en VS Code
- ESP32-WROOM-32D conectada por USB al puerto COM8
- ESP32 camara en la misma red local
- Servidor honeypot-iot-server en la misma red local

## Configuracion

Copia el fichero de ejemplo y rellena tus valores:

```bash
cp config/config.h.example config/config.h
```

Las variables que hay que ajustar:

- `WIFI_SSID` y `WIFI_PASSWORD` - credenciales de la red WiFi
- `SERVER_URL` - URL completa del endpoint /hardware/event del servidor
- `DEVICE_ID` - identificador del dispositivo en los eventos

## Despliegue

Conecta la ESP32 al puerto COM8, selecciona el entorno `esp32-plc-gateway` en PlatformIO y haz Upload. El monitor serie a 115200 baudios muestra el arranque WiFi, los eventos recibidos de la camara y los reenvios al servidor.

## Endpoints disponibles

- `POST /event` - recibe eventos de la ESP32 camara y los reenvía al servidor
- `GET /info` - informacion del controlador industrial simulado
- `GET /status` - estado del PLC
- `POST /command` - intento de comando industrial
- `POST /firmware/update` - actualizacion OTA
- Cualquier ruta no definida genera un evento `UNKNOWN_ENDPOINT`

## Flujo de datos

La camara manda eventos a `/event`. El PLC-Gateway los recibe, añade los campos `type: HoneypotIoT` y `source: hardware`, y los reenvía al servidor por HTTP POST. El endpoint `/hardware/event` del servidor esta excluido del middleware de FastAPI para evitar duplicados en Elasticsearch.

## Relacion con el resto del proyecto

Este firmware es el intermediario entre la ESP32 camara y el servidor. El repositorio de la camara es `esp32-camara` y el del servidor es `honeypot-iot-server`.