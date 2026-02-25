/* ======================================
 * File: error_control.h
 * Description: Centralized error handling and camera degradation
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef ERROR_CONTROL_H
#define ERROR_CONTROL_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "stdint.h"

typedef enum
{
    SEVERITY_DEBUG = 0,
    SEVERITY_INFO,
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    SEVERITY_CRITICAL
} error_severity_t;

typedef enum
{
    CAM_STATE_NORMAL = 0,
    CAM_STATE_DEGRADED,
    CAM_STATE_SUSPENDED
} camera_state_t;

#define CMD_DIFF_THRESHOLD_DEGRADE 6
#define CMD_DIFF_THRESHOLD_SUSPEND 10
#define CMD_DIFF_THRESHOLD_RESTORE 2

#define ERROR_JSON_MAX_LEN 256

/** Message sent to WebSocket task for error display. */
typedef struct
{
    char json_message[ERROR_JSON_MAX_LEN];
    uint16_t len;
} error_web_msg_t;

/**
 * @brief Initialize the error control module.
 * @return ESP_OK on success.
 */
esp_err_t error_control_init(void);

/**
 * @brief Stop the error control task.
 */
void error_control_stop(void);

/**
 * @brief Register that a command was received (for degradation tracking).
 */
void error_control_cmd_received(void);

/**
 * @brief Register that a command was sent successfully (for degradation tracking).
 */
void error_control_cmd_sent_ok(void);

/**
 * @brief Get current camera state (NORMAL, DEGRADED, SUSPENDED).
 * @return Current camera state.
 */
camera_state_t error_control_get_camera_state(void);

/**
 * @brief Set camera state (for testing or manual control).
 * @param state New camera state.
 */
void error_control_set_camera_state(camera_state_t state);

#endif
