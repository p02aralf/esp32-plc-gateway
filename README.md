# esp32-plc-gateway

Firmware para la ESP32-WROOM-32D que actúa como PLC-Gateway dentro del honeypot IoT. Tiene dos roles a la vez: recibe los eventos generados por la ESP32 cámara y los reenvía al servidor FastAPI, y expone sus propios endpoints simulando un controlador industrial que puede ser atacado directamente.

Es el intermediario entre el hardware de campo y el backend. Sin él, los eventos de la cámara no llegan al servidor y el sistema pierde la capa de realismo que aporta el hardware físico.

---

## Índice

1. [Requisitos](#1-requisitos)
2. [Estructura del repositorio](#2-estructura-del-repositorio)
3. [Configuración](#3-configuración)
   - 3.1 [Cómo obtener los valores de configuración](#31-cómo-obtener-los-valores-de-configuración)
4. [Despliegue](#4-despliegue)
5. [Endpoints disponibles](#5-endpoints-disponibles)
6. [Flujo de datos](#6-flujo-de-datos)
7. [Relación con el resto del proyecto](#7-relación-con-el-resto-del-proyecto)

---

## 1. Requisitos

- **PlatformIO** instalado en VS Code. Si no lo tienes, instala la extensión PlatformIO IDE desde el marketplace de VS Code.
- **ESP32-WROOM-32D** conectada por USB al ordenador.
- **Servidor honeypot-iot-server** corriendo y accesible en la red local.
- **Driver USB-to-UART** instalado. La mayoría de placas ESP32 usan el chip CP2102 o CH340. Si Windows no reconoce la placa al conectarla, instala el driver correspondiente:
  - CP2102: [silabs.com/developers/usb-to-uart-bridge-vcp-drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
  - CH340: [wch-ic.com/downloads/CH341SER_EXE.html](http://www.wch-ic.com/downloads/CH341SER_EXE.html)

---

## 2. Estructura del repositorio

```
esp32-plc-gateway/
│
├── config/
│   ├── config.h          # Configuración real con tus valores (no se sube a git)
│   └── config.h.example  # Plantilla vacía para configurar el entorno
│
├── src/
│   └── main.cpp          # Firmware completo
│
├── platformio.ini        # Configuración del proyecto PlatformIO
├── .gitignore
└── README.md
```

El fichero `config.h` nunca se sube al repositorio porque contiene credenciales WiFi e IPs de la red local. Lo que sí se sube es `config.h.example`, que es la plantilla vacía que sirve de referencia para configurar cualquier despliegue nuevo.

---

## 3. Configuración

Copia la plantilla y rellena tus valores:

```bash
cp config/config.h.example config/config.h
```

El fichero `config.h` tiene tres variables:

```cpp
#pragma once

// --- CONFIGURACION WiFi ----------------------------------------------
#define WIFI_SSID     "nombre_de_tu_red"
#define WIFI_PASSWORD "contraseña_de_tu_red"

// --- CONFIGURACION SERVIDOR ------------------------------------------
#define SERVER_URL "http://192.168.X.X:8888/hardware/event"

// --- IDENTIFICACION DEL DISPOSITIVO ----------------------------------
#define DEVICE_ID "esp32-plc-01"
```

### 3.1 Cómo obtener los valores de configuración

**`WIFI_SSID` y `WIFI_PASSWORD`**

Son el nombre y la contraseña de tu red WiFi. Los encuentras en la etiqueta del router o en la configuración de red de cualquier dispositivo ya conectado. Asegúrate de usar la red de 2.4 GHz, no la de 5 GHz, porque el chip WiFi del ESP32 no soporta 5 GHz.

**`SERVER_URL`**

Es la URL completa del endpoint `/hardware/event` del servidor FastAPI. Necesitas saber la IP del servidor en tu red local. Para encontrarla:

- En el servidor Ubuntu, ejecuta:
  ```bash
  ip a | grep "inet " | grep -v 127
  ```
  La IP que aparece junto a la interfaz de red (normalmente `ens33` o `eth0`) es la que tienes que usar.
- Si tienes acceso al panel de tu router, busca el dispositivo en la lista de clientes DHCP. Debería aparecer con el nombre de host configurado durante la instalación de Ubuntu.
- Usa una herramienta de escaneo de red como [Advanced IP Scanner](https://www.advanced-ip-scanner.com/) en Windows o `nmap -sn 192.168.X.0/24` en Linux/Mac.

La URL completa tiene siempre este formato:
```
http://<IP_DEL_SERVIDOR>:8888/hardware/event
```

El puerto 8888 es fijo, es donde escucha el contenedor Docker del honeypot-iot.

**`DEVICE_ID`**

Identificador libre que aparece en los eventos enviados al servidor. Se usa en Kibana para filtrar eventos por dispositivo. Puedes dejarlo como está o cambiarlo a algo más descriptivo para tu entorno.

**Nota sobre `SERVER_URL` y `PLC_GATEWAY_IP` en el servidor**

En el fichero `config/.env` del servidor hay una variable llamada `PLC_GATEWAY_IP` que tiene que coincidir con la IP que el router asigna a esta placa. El middleware de FastAPI usa esa IP para marcar los eventos entrantes como `source: hardware` en lugar de `source: virtual`. Si no coinciden, los eventos del PLC-Gateway llegarán a Kibana marcados incorrectamente como virtuales.

Para saber qué IP tiene asignada esta placa una vez flasheada, abre el monitor serie de PlatformIO a 115200 baudios. Al arrancar imprime su IP:
```
WiFi conectado. IP: 192.168.X.X
```
Esa IP es la que tienes que poner en `PLC_GATEWAY_IP` del `.env` del servidor.

**Cómo encontrar el puerto COM de la ESP32 en Windows**

1. Conecta la ESP32 por USB.
2. Abre el **Administrador de dispositivos** (Win+X → Administrador de dispositivos).
3. Despliega **Puertos (COM y LPT)**.
4. Busca algo como `Silicon Labs CP210x USB to UART Bridge (COM8)` o `USB-SERIAL CH340 (COM8)`.
5. Ese número de COM es el que tienes que poner en `platformio.ini` en los campos `monitor_port` y `upload_port`.

Si no aparece ningún dispositivo nuevo al conectar la ESP32, revisa los drivers (ver sección 1).

---

## 4. Despliegue

Con `config.h` listo y la ESP32 conectada:

1. Abre el proyecto en VS Code con PlatformIO.
2. En la barra inferior de PlatformIO selecciona el entorno `esp32-plc-gateway`.
3. Haz clic en **Upload** (el icono de flecha hacia la derecha).
4. Una vez subido el firmware, abre el **Monitor Serie** a 115200 baudios.

Deberías ver algo como esto al arrancar:

```
Conectando a WiFi...
WiFi conectado. IP: 192.168.X.X
Servidor HTTP iniciado en puerto 80
Listo para recibir eventos
```

Si el WiFi no conecta, verifica `WIFI_SSID` y `WIFI_PASSWORD` en `config.h`. Si el servidor no responde, verifica que `SERVER_URL` es correcta y que el contenedor honeypot-iot está corriendo.

---

## 5. Endpoints disponibles

Todos los endpoints están en el puerto 80. Los que recibe de la cámara (`/event`) los reenvía directamente al servidor. Los demás los expone como honeypot industrial y generan eventos propios.

| Método | Endpoint | Descripción |
|---|---|---|
| POST | `/event` | Recibe eventos de la ESP32 cámara y los reenvía al servidor. |
| GET | `/info` | Información del controlador industrial simulado. |
| GET | `/status` | Estado del PLC. |
| POST | `/command` | Intento de comando industrial. Usado en ataques de explotación OT. |
| POST | `/firmware/update` | Actualización OTA. Usado en ataques de post-explotación. |
| * | Cualquier otra ruta | Genera un evento `UNKNOWN_ENDPOINT`. |

---

## 6. Flujo de datos

```
ESP32 Cámara (192.168.X.X)
        │
        │ HTTP POST → PLC_IP:80/event
        ▼
  ESP32 PLC-Gateway (192.168.X.X)
        │
        │ Añade campos type: HoneypotIoT y source: hardware
        │ HTTP POST → SERVER_URL/hardware/event
        ▼
  Servidor honeypot-iot (192.168.X.X:8888)
        │
        │ HTTP POST → logstash:64305
        ▼
  Logstash → Elasticsearch → Kibana

Atacante o simulador
        │
        │ HTTP request directo (puerto 80)
        ▼
  ESP32 PLC-Gateway (192.168.X.X)
        │
        │ Genera evento propio y lo envía al servidor
        ▼
  Servidor honeypot-iot
```

El PLC-Gateway tiene dos fuentes de tráfico: los eventos que le llegan de la cámara y los ataques directos contra sus propios endpoints. Ambos terminan en el servidor marcados como `source: hardware`.

---

## 7. Relación con el resto del proyecto

Este firmware es el intermediario entre el hardware de campo y el servidor. Los otros dos componentes son:

- **esp32-camara** — genera eventos de cámara que llegan aquí antes de ir al servidor.
- **honeypot-iot-server** — servidor FastAPI con T-Pot y el stack ELK. Es el destino final de todos los eventos.