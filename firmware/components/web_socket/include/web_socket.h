/* ======================================
- File: web_socket.h
- Description: Header of the web socket logic.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-01-27
- ====================================== */

#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include "app_globals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json_parser.h"
#include "esp_websocket_client.h"
#include "esp_err.h"
#include "string.h"
#include "stdlib.h"
#include <stdint.h> 

esp_err_t websocket_start(void);
void websocket_stop();


#endif