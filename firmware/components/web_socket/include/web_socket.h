/* ======================================
 * File: web_socket.h
 * Description: WebSocket client for Rover backend
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "app_globals.h"
#include "json_parser.h"

/**
 * @brief Start the WebSocket client (connects to server from NVS/provisioning).
 * @return ESP_OK on success.
 */
esp_err_t websocket_start(void);

/**
 * @brief Stop the WebSocket client.
 */
void websocket_stop(void);

/**
 * @brief Force a reconnection attempt (stop then start client).
 */
void websocket_trigger_reconnect(void);

#endif