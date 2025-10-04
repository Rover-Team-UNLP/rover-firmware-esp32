# JSON Parser

## Descripción

Módulo para parsear comandos JSON y manejar un buffer circular de comandos para el rover ESP32-CAM. Permite recibir, almacenar, recuperar y modificar comandos de movimiento con parámetros.

## Interfaz

### Estructuras de datos

```c
// Tipos de comando
typedef enum {
    CMD_MOVE_FORWARD = 0,
    CMD_MOVE_BACKWARDS,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT
} rover_cmd_type_t;

// Comando individual
typedef struct {
    uint16_t id;                        // ID único del comando
    rover_cmd_type_t cmd;               // Tipo de comando
    double params[CMD_PARAMS_LEN];      // Parámetros (máximo 10)
    uint8_t total_params;               // Cantidad de parámetros
} data_cmd;

// Buffer circular de comandos
typedef struct {
    data_cmd buffer[CMD_BUFFER_LEN];    // Buffer (capacidad 10)
    uint16_t newest_id;                 // ID más reciente
    uint8_t count;                      // Cantidad actual de comandos
} cmd_buffer_t;
```

### Funciones principales

```c
// Parsear JSON y almacenar comando
json_parser_status_t parse_json(char *data, char *uart_string);

// Recuperar comando por ID
json_parser_status_t take_cmd(uint16_t id, data_cmd *command);

// Modificar comando existente
json_parser_status_t modify_cmd(uint16_t id, data_cmd *command);
```

## Funcionamiento

### 1. Parsing JSON

Acepta JSONs con formato:
```json
{
    "id": 123,
    "cmd": 0,
    "params": [1.5, 2.0, 3.5]
}
```

### 2. Generación de string UART

Convierte el comando a string para transmisión:
```
Input:  {"id": 5, "cmd": 1, "params": [1.5, 2.0]}
Output: "5-1-1.50-2.00\n"
```

### 3. Buffer circular

- **Capacidad:** 10 comandos
- **Comportamiento:** Al llenarse, sobrescribe comandos antiguos
- **Acceso:** Por ID único usando indexación modular

## Rendimiento ESP32-CAM (Teóricos)

| Métrica | Valor |
|---------|-------|
| **Tiempo de parsing** | ~0.2-0.3 ms |
| **Throughput máximo** | ~3,300 comandos/seg |
| **Memoria RAM** | ~400 bytes (buffer) |
| **Ciclos CPU** | ~48,000-72,000 @ 240MHz |

### Factores de rendimiento

- ✅ **Rápido:** Single-pass parsing, stack allocation
- ✅ **Eficiente:** Buffer circular, sin fragmentación de heap
- ✅ **Escalable:** Manejo de 0-10 parámetros por comando

## Testing

### Cobertura de tests

- ✅ **Parsing básico** - JSONs válidos con diferentes comandos
- ✅ **Manejo de errores** - JSONs malformados, campos faltantes
- ✅ **Buffer operations** - take_cmd, modify_cmd, overflow
- ✅ **Edge cases** - Sin parámetros, máximo parámetros

### Ejemplo de uso

```c
#include "json_parser.h"

// Parsear comando
char *json = "{\"id\": 1, \"cmd\": 0, \"params\": [1.5]}";
char uart_output[256];
parse_json(json, uart_output);

// Recuperar comando
data_cmd cmd;
take_cmd(1, &cmd);

// Modificar comando
cmd.params[0] = 2.0;
modify_cmd(1, &cmd);
```

## Estados de retorno

```c
typedef enum {
    STATUS_OK = 0,              // Operación exitosa
    STATUS_PARSE_ERROR,         // Error de parsing JSON
    STATUS_ID_NOT_FOUND,        // ID no encontrado en buffer
    STATUS_BUFFER_CMD_NULL,     // Puntero NULL
    STATUS_NOT_VALID_ID         // ID inválido (0)
} json_parser_status_t;
```

## Casos de uso típicos

1. **Control remoto:** Recibir comandos vía WiFi/UART
2. **Buffer de comandos:** Almacenar secuencias de movimientos
3. **Telemetría:** Generar strings para transmisión
4. **Debugging:** Modificar comandos en runtime

---

*Desarrollado para Rover Team UNLP - ESP32-CAM Firmware*
