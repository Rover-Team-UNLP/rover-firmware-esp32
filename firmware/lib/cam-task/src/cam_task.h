/* ======================================
- File: cam_task.h
- Description: header of streaming of MJPEG capture with a Cam using a HTTP server
- Author/s: @JuanCruzFerreiraM
- Last-update: 2025-10-04
- ====================================== */
#ifndef CAM_TASK
#define CAM_TASK

#include "esp_camera.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include "app_globals.h"

#define Y2_GPIO_NUM 5
#define Y3_GPIO_NUM 18
#define Y4_GPIO_NUM 19
#define Y5_GPIO_NUM 21
#define Y6_GPIO_NUM 36
#define Y7_GPIO_NUM 39
#define Y8_GPIO_NUM 34
#define Y9_GPIO_NUM 35
#define XCLK_GPIO_NUM 0
#define PCLK_GPIO_NUM 22
#define RESET_GPIO_NUM -1
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define PWDN_GPIO_NUM 32

const char *url = "url_for_server";


/**
 * @brief This function initialize the camera using the camera_init() function define at esp_camera.h with the configuration for JPEG at 20FPS.
 *
 * @return esp_err_t. Error code from the function camera_init(), will return ESP_OK if the initialization was successful.
 */
esp_err_t start_camera();

void camera_task(void);

#endif