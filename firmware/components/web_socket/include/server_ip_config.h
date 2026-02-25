/* ======================================
 * File: server_ip_config.h
 * Description: Server IP retrieval from NVS (used by WebSocket/camera)
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef SERVER_IP_CONFIG_H
#define SERVER_IP_CONFIG_H

#include <stddef.h>
#include "esp_err.h"

#define MAX_SERVER_IP_LENGTH 16

/**
 * @brief Retrieve server IP from NVS (set during WiFi provisioning).
 * @param ip      Buffer to store the IP string.
 * @param max_len Size of the buffer.
 * @return ESP_OK if found; ESP_ERR_NVS_NOT_FOUND if not set (caller may use default).
 */
esp_err_t storage_get_server_ip(char *ip, size_t max_len);

#endif
