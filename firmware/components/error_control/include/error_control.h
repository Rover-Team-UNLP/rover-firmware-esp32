/* ======================================
- File: error_control.h
- Description: Error control module for centralized error handling and camera degradation
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */

#ifndef ERROR_CONTROL_H
#define ERROR_CONTROL_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "stdint.h"

/* ============== SEVERITY LEVELS ============== */
typedef enum
{
    SEVERITY_DEBUG = 0, // Solo log interno, errores esperados/frecuentes
    SEVERITY_INFO,      // Log + opcional notificar
    SEVERITY_WARNING,   // Log + notificar web
    SEVERITY_ERROR,     // Log + notificar + posible acción
    SEVERITY_CRITICAL   // Log + notificar + detener operación
} error_severity_t;

/* ============== CAMERA STATES ============== */
typedef enum
{
    CAM_STATE_NORMAL = 0, // Funcionamiento normal
    CAM_STATE_DEGRADED,   // Calidad/FPS reducido
    CAM_STATE_SUSPENDED   // Cámara suspendida
} camera_state_t;

/* ============== THRESHOLDS ============== */
#define CMD_DIFF_THRESHOLD_DEGRADE 5  // Diferencia para degradar cámara
#define CMD_DIFF_THRESHOLD_SUSPEND 10 // Diferencia para suspender cámara
#define CMD_DIFF_THRESHOLD_RESTORE 2  // Diferencia para restaurar cámara

/* ============== ERROR MESSAGE FOR WEB ============== */
#define ERROR_JSON_MAX_LEN 256

/**
 * @brief Estructura para enviar errores a la tarea de WebSocket
 */
typedef struct
{
    char json_message[ERROR_JSON_MAX_LEN];
    uint16_t len;
} error_web_msg_t;

/* ============== PUBLIC API ============== */

/**
 * @brief Inicializa el módulo de control de errores
 * @return ESP_OK si la inicialización fue exitosa
 */
esp_err_t error_control_init(void);

/**
 * @brief Detiene el módulo de control de errores
 */
void error_control_stop(void);

/**
 * @brief Registra un comando recibido (para tracking de degradación)
 */
void error_control_cmd_received(void);

/**
 * @brief Registra un comando enviado exitosamente (para tracking de degradación)
 */
void error_control_cmd_sent_ok(void);

/**
 * @brief Obtiene el estado actual de la cámara
 * @return Estado actual de la cámara
 */
camera_state_t error_control_get_camera_state(void);

/**
 * @brief Fuerza un estado de cámara (para testing o control manual)
 * @param state Nuevo estado de la cámara
 */
void error_control_set_camera_state(camera_state_t state);

#endif /* ERROR_CONTROL_H */
