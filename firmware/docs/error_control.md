# Error Control

## 📋 Descripción

Módulo centralizado para el manejo de errores del sistema ESP32-CAM Rover. Recibe errores de todos los módulos (UART, WebSocket, Cámara, JSON Parser), los clasifica por severidad, ejecuta políticas de respuesta y notifica a la aplicación web. Incluye un sistema de **degradación automática de cámara** basado en el flujo de comandos.

## 🔧 Interfaz

### Estructuras de datos

```c
// Niveles de severidad
typedef enum {
    SEVERITY_DEBUG = 0,     // Solo log interno
    SEVERITY_INFO,          // Log + opcional notificar
    SEVERITY_WARNING,       // Log + notificar web
    SEVERITY_ERROR,         // Log + notificar + acción
    SEVERITY_CRITICAL       // Log + notificar + detener
} error_severity_t;

// Estados de la cámara
typedef enum {
    CAM_STATE_NORMAL = 0,   // Funcionamiento normal
    CAM_STATE_DEGRADED,     // Calidad/FPS reducido
    CAM_STATE_SUSPENDED     // Cámara suspendida
} camera_state_t;

// Mensaje de error para WebSocket
typedef struct {
    char json_message[256]; // JSON formateado
    uint16_t len;           // Longitud del mensaje
} error_web_msg_t;
```

### Funciones principales

```c
// Inicializar módulo de control de errores
esp_err_t error_control_init(void);

// Detener el módulo
void error_control_stop(void);

// Registrar comando recibido (para tracking)
void error_control_cmd_received(void);

// Registrar comando enviado OK (para tracking)
void error_control_cmd_sent_ok(void);

// Obtener estado actual de la cámara
camera_state_t error_control_get_camera_state(void);

// Forzar estado de cámara (manual/testing)
void error_control_set_camera_state(camera_state_t state);
```

## 🚀 Funcionamiento

### 1. Arquitectura del Sistema

```
┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│   CAMERA    │  │    UART     │  │ JSON_PARSER │  │  WEBSOCKET  │
└──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘
       │                │                │                │
       └────────────────┴───────┬────────┴────────────────┘
                                ▼
                       ┌─────────────────┐
                       │ to_error_queue  │
                       └────────┬────────┘
                                ▼
              ┌─────────────────────────────────┐
              │       ERROR_CONTROL_TASK        │
              │                                 │
              │  1. Clasifica error (severidad) │
              │  2. Log local (siempre)         │
              │  3. Notifica web (si aplica)    │
              │  4. Evalúa estado de cámara     │
              └─────────────────┬───────────────┘
                                │
                                ▼
                       ┌─────────────────┐
                       │ from_error_queue│ ──► WS_SEND_TASK ──► Web
                       └─────────────────┘
```

### 2. Flujo de Procesamiento

1. **Recepción:** Un módulo envía `Error_inf` a `to_error_queue`
2. **Clasificación:** Se determina severidad según el tipo de error
3. **Logging:** Se hace log local con nivel apropiado (DEBUG/INFO/WARN/ERROR)
4. **Notificación:** Si severidad ≥ WARNING, se envía JSON a `from_error_queue`
5. **Envío Web:** La tarea `send_error_task` en WebSocket envía el JSON al servidor

### 3. Sistema de Degradación de Cámara

El módulo monitorea la diferencia entre comandos recibidos y comandos enviados exitosamente:

```
diff = cmds_received - cmds_sent_ok
```

| Condición | Estado Resultante | Acción |
|-----------|-------------------|--------|
| diff < 5 | `CAM_STATE_NORMAL` | Cámara normal (QVGA, 25 FPS) |
| diff ≥ 5 | `CAM_STATE_DEGRADED` | Reduce calidad (QQVGA, 10 FPS) |
| diff ≥ 10 | `CAM_STATE_SUSPENDED` | Suspende cámara |
| diff ≤ 2 | Restaurar | Vuelve al estado anterior |

**Thresholds configurables:**
```c
#define CMD_DIFF_THRESHOLD_DEGRADE  5   // Diferencia para degradar
#define CMD_DIFF_THRESHOLD_SUSPEND  10  // Diferencia para suspender
#define CMD_DIFF_THRESHOLD_RESTORE  2   // Diferencia para restaurar
```

## 📊 Catálogo de Errores

### Errores de Cámara

| Código | Nombre | Severidad | Descripción |
|--------|--------|-----------|-------------|
| 0 | `HTTP_CLIENT_NO_OPEN` | ERROR | HTTP client falló al abrir conexión |
| 1 | `UL_STATUS_FAIL` | WARNING | Verificación de estado de upload falló |
| 2 | `FRAME_NULL` | DEBUG | Frame buffer de cámara fue NULL |
| 3 | `WRITE_ERROR` | WARNING | Falló escritura al stream |
| 4 | `CAMERA_INIT_FAIL` | CRITICAL | Inicialización de cámara falló |

### Errores de UART

| Código | Nombre | Severidad | Descripción |
|--------|--------|-----------|-------------|
| 5 | `NO_ACK` | WARNING | Sin ACK de CIAA después de reintentos |
| 6 | `UART_INVALID_CMD` | WARNING | CIAA reportó comando inválido |
| 7 | `UART_INVALID_PARAMS` | WARNING | CIAA reportó parámetros inválidos |
| 8 | `UART_UNKNOWN_RESPONSE` | INFO | Respuesta desconocida de CIAA |
| 9 | `UART_PARSE_FAIL` | WARNING | Falló parseo de respuesta UART |

### Errores de WebSocket

| Código | Nombre | Severidad | Descripción |
|--------|--------|-----------|-------------|
| 10 | `NULL_BUFFER` | ERROR | Allocación de memoria falló |
| 11 | `PARSE_ERROR` | INFO | Falló parseo de comando JSON |
| 12 | `SEND_CMD_QUEUE_ERROR` | ERROR | Falló envío a cola de comandos |
| 13 | `OP_CODE_ERROR` | INFO | Opcode WebSocket inválido |
| 14 | `WEBSOCKET_DISCONNECTED` | ERROR | Conexión WebSocket perdida |
| 15 | `WEBSOCKET_INIT_FAIL` | CRITICAL | Inicialización de WS falló |
| 16 | `WEBSOCKET_REGISTER_FAIL` | CRITICAL | Registro de eventos WS falló |
| 17 | `WEBSOCKET_START_FAIL` | CRITICAL | Inicio de cliente WS falló |

### Errores de Sistema

| Código | Nombre | Severidad | Descripción |
|--------|--------|-----------|-------------|
| 18 | `WIFI_CONNECTION_FAIL` | ERROR | Conexión WiFi falló |
| 19 | `QUEUE_CREATE_FAIL` | CRITICAL | Creación de cola FreeRTOS falló |

## 📤 Formato JSON para Web

Los errores con severidad ≥ WARNING se envían al servidor web con este formato:

```json
{
  "type": "error",
  "ts": 123456789,
  "source": "CAMERA",
  "code": 3,
  "name": "WRITE_ERROR",
  "severity": "warning",
  "msg": "Failed to write data to stream",
  "id": 42
}
```

### Campos del JSON

| Campo | Tipo | Descripción |
|-------|------|-------------|
| `type` | string | Siempre `"error"` |
| `ts` | number | Timestamp en ms desde boot |
| `source` | string | `"UART"`, `"JSON_PARSER"`, `"CAMERA"`, `"WEB_SOCKET"` |
| `code` | number | Código numérico del error |
| `name` | string | Nombre legible del error |
| `severity` | string | `"debug"`, `"info"`, `"warning"`, `"error"`, `"critical"` |
| `msg` | string | Descripción del error en inglés |
| `id` | number | ID de contexto (ej: ID del comando que falló) |

## 🔄 Integración con Otros Módulos

### WebSocket (`web_socket.c`)

```c
// Al recibir un comando válido:
error_control_cmd_received();

// Tarea que envía errores a la web:
static void send_error_task(void *pvParameters) {
    error_web_msg_t error_msg;
    while (1) {
        if (xQueueReceive(from_error_queue, &error_msg, portMAX_DELAY)) {
            if (esp_websocket_client_is_connected(client)) {
                esp_websocket_client_send_text(client, error_msg.json_message, ...);
            }
        }
    }
}
```

### UART (`uart_task.c`)

```c
// Al recibir ACK de la CIAA:
error_control_cmd_sent_ok();
```

### Cámara (`cam_task.c`)

```c
// En cada iteración del loop de streaming:
camera_state_t state = error_control_get_camera_state();

if (state == CAM_STATE_SUSPENDED) {
    // Esperar hasta que se restaure
    while (error_control_get_camera_state() == CAM_STATE_SUSPENDED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Ajustar calidad según estado
camera_adjust_quality(state);
```

## ⚡ Rendimiento

| Métrica | Valor |
|---------|-------|
| **Stack de tarea** | 4096 bytes |
| **Prioridad** | 5 |
| **Latencia de procesamiento** | < 1 ms |
| **Tamaño máximo JSON** | 256 bytes |
| **Capacidad de colas** | 10 mensajes |

## 🧪 Ejemplo de Uso

### Enviar un error desde cualquier módulo

```c
#include "app_globals.h"

void some_function() {
    // Detectar error
    if (something_failed) {
        Error_inf error = {
            .id = operation_id,
            .source = CAMERA,  // o UART, WEB_SOCKET, JSON_PARSER
            .response_type = 0,
            .general_errors = WRITE_ERROR
        };
        xQueueSend(to_error_queue, &error, 0);
    }
}
```

### Consultar estado de cámara

```c
#include "error_control.h"

void check_camera_status() {
    camera_state_t state = error_control_get_camera_state();
    
    switch (state) {
        case CAM_STATE_NORMAL:
            printf("Cámara funcionando normalmente\n");
            break;
        case CAM_STATE_DEGRADED:
            printf("Cámara en modo degradado\n");
            break;
        case CAM_STATE_SUSPENDED:
            printf("Cámara suspendida\n");
            break;
    }
}
```

## 📁 Archivos del Módulo

```
components/error_control/
├── CMakeLists.txt
├── include/
│   └── error_control.h      # API pública y tipos
└── src/
    └── error_control.c      # Implementación
```

## 🔧 Configuración en CMakeLists

```cmake
idf_component_register(
    SRCS "src/error_control.c"
    INCLUDE_DIRS 
        "include"
        "../../include"
    REQUIRES 
        freertos 
        esp_common 
        esp_timer
        esp_log
)
```

---

*Desarrollado para Rover Team UNLP - ESP32-CAM Firmware*
