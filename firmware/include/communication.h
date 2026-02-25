/* ======================================
 * File: communication.h
 * Description: Macros and structs for ESP32-CIAA UART communication
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define ACK_AWAIT_MS 200
#define CMD_AWAIT_MS 1000
#define RESPONSE_LEN 13
#define CMD_LEN 20

/** Response format: S:RESP:ID:E */
static const char *response_format = "%c:%d:%d:%c";
/** Command format: S:CMD:INTENSITY:ID:E */
static const char *cmd_format = "S:%u:%u:%u:E";
#define CMD_START 'S'
#define CMD_END 'E'

/** Response IDs from EDU-CIAA. */
typedef enum
{
    RESP_ACK = 0,
    RESP_READY,
    RESP_NACK,
    RESP_ERR_INVALID_COMMAND,
    RESP_ERR_INVALID_PARAMS,
    RESP_COUNT
} uart_resp_id_t;

/** Rover motion command types. */
typedef enum
{
    CMD_MOVE_FORWARD = 0,
    CMD_MOVE_BACKWARDS,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
    CMD_STOP = 4
} rover_cmd_type_t;

typedef struct
{
    uint16_t id;
    rover_cmd_type_t cmd;
    uint8_t intensity; /**< Command intensity 0-255 */
} data_cmd;

#endif