/* ======================================
 * File: http_portal.h
 * Description: HTTP captive portal for WiFi provisioning
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef HTTP_PORTAL_H
#define HTTP_PORTAL_H

#include "esp_http_server.h"
#include "esp_err.h"
#include "nvs_storage.h"

typedef struct
{
    httpd_handle_t handler;
    esp_err_t return_err;
} server_start_return_t;

/**
 * @brief Start the provisioning web server (port 80, GET/POST handlers).
 * @return Struct with server handle and status; handler is NULL on failure.
 */
server_start_return_t http_portal_start(void);

/**
 * @brief Stop the provisioning web server and free resources.
 * @param handle Server handle returned by http_portal_start().
 */
void http_portal_stop(httpd_handle_t handle);

#endif