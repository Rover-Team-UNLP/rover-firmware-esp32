/* ======================================
- File: cam_task.h
- Description: header of streaming of MJPEG capture with a Cam using a HTTP server
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */
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

// URL del servidor para streaming (definida en cam_task.c)
extern const char *CAM_SERVER_URL;

/* ============== CAMERA QUALITY SETTINGS ============== */
// Calidad inicial al abrir la cámara (debe coincidir con modo normal para evitar salto visual)
#define CAM_JPEG_QUALITY_INIT 8
// Normal mode: QVGA (320x240), quality 8 (mejor imagen), ~25 FPS (40ms delay) - sin subir carga
#define CAM_NORMAL_FRAMESIZE FRAMESIZE_QVGA
#define CAM_NORMAL_QUALITY 8
#define CAM_NORMAL_DELAY_MS 40

// Degraded mode: QQVGA (160x120), quality 15, 10 FPS (100ms delay)
#define CAM_DEGRADED_FRAMESIZE FRAMESIZE_QQVGA
#define CAM_DEGRADED_QUALITY 15
#define CAM_DEGRADED_DELAY_MS 100

/**
 * @brief This function initialize the camera using the camera_init() function define at esp_camera.h with the configuration for JPEG at 20FPS.
 *
 * @return esp_err_t. Error code from the function camera_init(), will return ESP_OK if the initialization was successful.
 */
esp_err_t start_camera(void);

/**
 * @brief Main camera streaming task
 */
void camera_task(void *pvParameters);

/**
 * @brief Ajusta la calidad de la cámara según el estado del sistema
 * @param state Estado actual de la cámara desde error_control
 */
void camera_adjust_quality(camera_state_t state);

#endif