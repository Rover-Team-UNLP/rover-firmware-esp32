/* ======================================
 * File: dns_server.h
 * Description: DNS captive-portal server (UDP port 53, responds with AP IP)
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/**
 * @brief Start the DNS captive-portal server (FreeRTOS task, UDP port 53).
 * Answers all A-record queries with the SoftAP IP (e.g. 192.168.4.1).
 * @return ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t dns_server_start(void);

/**
 * @brief Stop the DNS server and free resources (call after credentials saved).
 */
void dns_server_stop(void);

#endif