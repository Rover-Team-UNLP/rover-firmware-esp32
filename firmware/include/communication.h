/* ======================================
- File: communication.h
- Description: Header file with macros and structs for ESP32-CIAA communication
- Author/s: @JuanCruzFerreiraM, @taciano-pacchialat
- Last-update: 2025-10-20
- ====================================== */
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define ACK_AWAIT_MS 200 // Deben definirse los tiempos en la propia fase de pruebas
#define CMD_AWAIT_MS 1000
#define RESPONSE_LEN 13
#define CMD_LEN 20

// Formato de respuesta: S:RESP:ID:E
static const char *response_format = "%c:%d:%d:%c";
// Formato de comando: S:CMD:INTENSITY:ID:E
static const char *cmd_format = "S:%u:%u:%u:E";
#define CMD_START 'S'
#define CMD_END 'E'

/* IDs de respuestas/envíos desde EDU-CIAA */
typedef enum
{
    RESP_ACK = 0,
    RESP_READY,
    RESP_NACK,
    RESP_ERR_INVALID_COMMAND,
    RESP_ERR_INVALID_PARAMS,
    RESP_COUNT
} uart_resp_id_t;

typedef enum
{
    CMD_MOVE_FORWARD = 0,
    CMD_MOVE_BACKWARDS,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT
} rover_cmd_type_t;

typedef struct
{
    uint16_t id;
    rover_cmd_type_t cmd;
    uint8_t intensity; // Intensidad del comando (0-255)
} data_cmd;

// Falta definir las estructuras de comandos, y otras cosas necesarias que vayan surgiendo.

#endif