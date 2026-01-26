#ifndef HTTP_PORTAL_H
#define HTTP_PORTAL_H

#include "esp_http_server.h"
#include "esp_err.h"
#include "nvs_storage.h"

typedef struct {
    httpd_handle_t handler;
    esp_err_t return_err;
} server_start_return_t; 



/**
 * @brief Starts the web server for WiFi provisioning.
 * * This function allocates memory for the server instance, starts the
 * HTTP daemon on port 80, and registers the URI handlers for the
 * configuration page (GET) and the credential submission (POST).
 *
 * @return
 * - httpd_handle_t: A handle to the server instance if successful.
 * - NULL: If the server failed to start.
 */
server_start_return_t http_portal_start(void);

/**
 * @brief Stops the provisioning web server.
 * * Frees all resources, closes active sockets, and deletes the
 * server handle. This should be called once credentials are
 * received and saved to NVS to free RAM for the camera stream.
 * * @param handle The handle of the server to stop.
 */
void http_portal_stop(httpd_handle_t handle);

#endif