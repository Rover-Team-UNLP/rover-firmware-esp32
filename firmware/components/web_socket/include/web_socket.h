/* ======================================
- File: web_socket.h
- Description: Header of the web socket logic.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-01-27
- ====================================== */

#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

// 1. Primero los headers estándar de C
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// 2. Luego los headers de ESP-IDF/FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_event.h"

// 3. Después el cliente WebSocket
#include "esp_websocket_client.h"

// 4. Finalmente los headers locales del proyecto
#include "app_globals.h"
#include "json_parser.h"

esp_err_t websocket_start(void);
void websocket_stop();


#endif