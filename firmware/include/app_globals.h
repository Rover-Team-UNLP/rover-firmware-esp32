/* ======================================
- File: app_globals.h
- Description: Global variables and types for the app
- Author/s: @JuanCruzFerreiraM
- Last-update: 2025-10-20
- ====================================== */

#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include "freertos/queue.h"
#include "communication.h"
#include "stdint.h"

extern QueueHandle_t to_error_queue;
extern QueueHandle_t from_error_queue;
extern QueueHandle_t cmd_queue;

typedef enum
{
    UART = 0,
    JSON_PARSER,
    CAMERA,
    WEB_SOCKET,
} source_type_t;

typedef enum
{
    HTTP_CLIENT_NO_OPEN = 0,
    UL_STATUS_FAIL,
    FRAME_NULL, 
    WRITE_ERROR,

} rover_errors_t;

typedef struct
{
    source_type_t source;
    uint16_t id;
    uart_resp_id_t response_type;
    rover_errors_t general_errors;
} Error_inf;

#endif