/* ======================================
 * File: cam_task.h
 * Description: MJPEG camera capture and HTTP upload task
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */
#ifndef CAM_TASK
#define CAM_TASK

#include "esp_camera.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include "app_globals.h"
#include "error_control.h"

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

extern const char *CAM_SERVER_URL;

/**
 * @brief Set the server URL for video upload. Call before start_camera or camera_task.
 * @param url Full URL (e.g. "http://192.168.1.34:8080/video/upload").
 */
void cam_task_set_server_url(const char *url);

#define CAM_JPEG_QUALITY_INIT 10
#define CAM_NORMAL_FRAMESIZE FRAMESIZE_QQVGA
#define CAM_NORMAL_QUALITY 10
#define CAM_NORMAL_DELAY_MS 40

#define CAM_DEGRADED_FRAMESIZE FRAMESIZE_QQVGA
#define CAM_DEGRADED_QUALITY 15
#define CAM_DEGRADED_DELAY_MS 100

/**
 * @brief Initialize the camera (esp_camera with JPEG config).
 * @return ESP_OK on success.
 */
esp_err_t start_camera(void);

/**
 * @brief Main camera capture task (creates upload task and runs capture loop).
 * @param pvParameters Unused.
 */
void camera_task(void *pvParameters);

/**
 * @brief Adjust camera quality and frame size based on system state.
 * @param state Current camera state from error_control.
 */
void camera_adjust_quality(camera_state_t state);

#endif
