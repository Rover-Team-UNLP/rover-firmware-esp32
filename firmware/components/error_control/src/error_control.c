/* ======================================
- File: error_control.c
- Description: Implementation of centralized error handling and camera degradation
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */

#include "error_control.h"
#include "app_globals.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "ERROR_CTRL";

/* ============== TASK CONFIG ============== */
#define ERROR_TASK_STACK_SIZE 4096
#define ERROR_TASK_PRIORITY 5

/* ============== PRIVATE DATA ============== */
static TaskHandle_t error_task_handle = NULL;
static volatile uint32_t cmds_received = 0;
static volatile uint32_t cmds_sent_ok = 0;
static volatile camera_state_t current_camera_state = CAM_STATE_NORMAL;

/* ============== ERROR METADATA ============== */
typedef struct
{
    const char *name;
    const char *message;
    error_severity_t severity;
} error_metadata_t;

/**
 * @brief Mapa de metadatos para cada tipo de error
 * IMPORTANTE: Mantener sincronizado con rover_errors_t en app_globals.h
 */
static const error_metadata_t error_map[] = {
    // Camera errors
    [HTTP_CLIENT_NO_OPEN] = {"HTTP_CLIENT_NO_OPEN", "HTTP client failed to open connection", SEVERITY_ERROR},
    [UL_STATUS_FAIL] = {"UL_STATUS_FAIL", "Upload status check failed", SEVERITY_WARNING},
    [FRAME_NULL] = {"FRAME_NULL", "Camera frame buffer was null", SEVERITY_DEBUG},
    [WRITE_ERROR] = {"WRITE_ERROR", "Failed to write data to stream", SEVERITY_WARNING},
    [CAMERA_INIT_FAIL] = {"CAMERA_INIT_FAIL", "Camera initialization failed", SEVERITY_CRITICAL},

    // UART errors
    [NO_ACK] = {"NO_ACK", "No ACK received from CIAA after retries", SEVERITY_WARNING},
    [UART_INVALID_CMD] = {"UART_INVALID_CMD", "CIAA reported invalid command", SEVERITY_WARNING},
    [UART_INVALID_PARAMS] = {"UART_INVALID_PARAMS", "CIAA reported invalid parameters", SEVERITY_WARNING},
    [UART_UNKNOWN_RESPONSE] = {"UART_UNKNOWN_RESPONSE", "Unknown response from CIAA", SEVERITY_INFO},
    [UART_PARSE_FAIL] = {"UART_PARSE_FAIL", "Failed to parse UART response", SEVERITY_WARNING},

    // WebSocket errors
    [NULL_BUFFER] = {"NULL_BUFFER", "Memory allocation failed", SEVERITY_ERROR},
    [PARSE_ERROR] = {"PARSE_ERROR", "Failed to parse JSON command", SEVERITY_INFO},
    [SEND_CMD_QUEUE_ERROR] = {"SEND_CMD_QUEUE_ERROR", "Failed to send command to queue", SEVERITY_ERROR},
    [OP_CODE_ERROR] = {"OP_CODE_ERROR", "Invalid WebSocket opcode received", SEVERITY_INFO},
    [WEBSOCKET_DISCONNECTED] = {"WEBSOCKET_DISCONNECTED", "WebSocket connection lost", SEVERITY_ERROR},
    [WEBSOCKET_INIT_FAIL] = {"WEBSOCKET_INIT_FAIL", "WebSocket client initialization failed", SEVERITY_CRITICAL},
    [WEBSOCKET_REGISTER_FAIL] = {"WEBSOCKET_REGISTER_FAIL", "Failed to register WebSocket events", SEVERITY_CRITICAL},
    [WEBSOCKET_START_FAIL] = {"WEBSOCKET_START_FAIL", "WebSocket client failed to start", SEVERITY_CRITICAL},

    // WiFi errors
    [WIFI_CONNECTION_FAIL] = {"WIFI_CONNECTION_FAIL", "WiFi connection failed after retries", SEVERITY_ERROR},

    // System errors
    [QUEUE_CREATE_FAIL] = {"QUEUE_CREATE_FAIL", "Failed to create FreeRTOS queue", SEVERITY_CRITICAL},
};

#define ERROR_MAP_SIZE (sizeof(error_map) / sizeof(error_map[0]))

/* ============== SOURCE NAMES ============== */
static const char *source_names[] = {
    [UART] = "UART",
    [JSON_PARSER] = "JSON_PARSER",
    [CAMERA] = "CAMERA",
    [WEB_SOCKET] = "WEB_SOCKET",
};

/* ============== SEVERITY NAMES ============== */
static const char *severity_names[] = {
    [SEVERITY_DEBUG] = "debug",
    [SEVERITY_INFO] = "info",
    [SEVERITY_WARNING] = "warning",
    [SEVERITY_ERROR] = "error",
    [SEVERITY_CRITICAL] = "critical",
};

/* ============== PRIVATE FUNCTIONS ============== */

/**
 * @brief Obtiene el timestamp actual en milisegundos
 */
static uint64_t get_timestamp_ms(void)
{
    return esp_timer_get_time() / 1000;
}

/**
 * @brief Construye el mensaje JSON para enviar a la web
 */
static void build_error_json(const Error_inf *error, char *buffer, size_t buffer_size)
{
    const error_metadata_t *meta = NULL;

    if (error->general_errors < ERROR_MAP_SIZE)
    {
        meta = &error_map[error->general_errors];
    }

    const char *source_name = (error->source < sizeof(source_names) / sizeof(source_names[0]))
                                  ? source_names[error->source]
                                  : "UNKNOWN";

    const char *error_name = meta ? meta->name : "UNKNOWN_ERROR";
    const char *error_msg = meta ? meta->message : "Unknown error occurred";
    const char *severity = meta ? severity_names[meta->severity] : "error";

    snprintf(buffer, buffer_size,
             "{"
             "\"type\":\"error\","
             "\"ts\":%llu,"
             "\"source\":\"%s\","
             "\"code\":%d,"
             "\"name\":\"%s\","
             "\"severity\":\"%s\","
             "\"msg\":\"%s\","
             "\"id\":%u"
             "}",
             get_timestamp_ms(),
             source_name,
             error->general_errors,
             error_name,
             severity,
             error_msg,
             error->id);
}

/**
 * @brief Evalúa y actualiza el estado de la cámara basado en el flujo de comandos
 */
static void evaluate_camera_state(void)
{
    int32_t cmd_diff = (int32_t)cmds_received - (int32_t)cmds_sent_ok;
    camera_state_t new_state = current_camera_state;

    // Evaluar transiciones de estado
    switch (current_camera_state)
    {
    case CAM_STATE_NORMAL:
        if (cmd_diff >= CMD_DIFF_THRESHOLD_SUSPEND)
        {
            new_state = CAM_STATE_SUSPENDED;
            ESP_LOGW(TAG, "Camera SUSPENDED: cmd_diff=%ld (threshold=%d)", cmd_diff, CMD_DIFF_THRESHOLD_SUSPEND);
        }
        else if (cmd_diff >= CMD_DIFF_THRESHOLD_DEGRADE)
        {
            new_state = CAM_STATE_DEGRADED;
            ESP_LOGW(TAG, "Camera DEGRADED: cmd_diff=%ld (threshold=%d)", cmd_diff, CMD_DIFF_THRESHOLD_DEGRADE);
        }
        break;

    case CAM_STATE_DEGRADED:
        if (cmd_diff >= CMD_DIFF_THRESHOLD_SUSPEND)
        {
            new_state = CAM_STATE_SUSPENDED;
            ESP_LOGW(TAG, "Camera DEGRADED->SUSPENDED: cmd_diff=%ld", cmd_diff);
        }
        else if (cmd_diff <= CMD_DIFF_THRESHOLD_RESTORE)
        {
            new_state = CAM_STATE_NORMAL;
            ESP_LOGI(TAG, "Camera DEGRADED->NORMAL: cmd_diff=%ld (restored)", cmd_diff);
        }
        break;

    case CAM_STATE_SUSPENDED:
        if (cmd_diff <= CMD_DIFF_THRESHOLD_RESTORE)
        {
            new_state = CAM_STATE_DEGRADED; // Primero a degraded, luego a normal
            ESP_LOGI(TAG, "Camera SUSPENDED->DEGRADED: cmd_diff=%ld (recovering)", cmd_diff);
        }
        break;
    }

    if (new_state != current_camera_state)
    {
        current_camera_state = new_state;
        // TODO: Aquí se puede notificar a la tarea de cámara para que ajuste
    }
}

/**
 * @brief Procesa un error recibido
 */
static void process_error(const Error_inf *error)
{
    const error_metadata_t *meta = NULL;

    if (error->general_errors < ERROR_MAP_SIZE)
    {
        meta = &error_map[error->general_errors];
    }

    error_severity_t severity = meta ? meta->severity : SEVERITY_ERROR;
    const char *error_name = meta ? meta->name : "UNKNOWN";

    // 1. SIEMPRE hacer log local
    switch (severity)
    {
    case SEVERITY_DEBUG:
        ESP_LOGD(TAG, "[%s] %s (id=%u)",
                 source_names[error->source], error_name, error->id);
        break;
    case SEVERITY_INFO:
        ESP_LOGI(TAG, "[%s] %s (id=%u)",
                 source_names[error->source], error_name, error->id);
        break;
    case SEVERITY_WARNING:
        ESP_LOGW(TAG, "[%s] %s (id=%u)",
                 source_names[error->source], error_name, error->id);
        break;
    case SEVERITY_ERROR:
    case SEVERITY_CRITICAL:
        ESP_LOGE(TAG, "[%s] %s (id=%u)",
                 source_names[error->source], error_name, error->id);
        break;
    }

    // 2. Si es WARNING o superior, notificar a la web
    if (severity >= SEVERITY_WARNING)
    {
        error_web_msg_t web_msg;
        build_error_json(error, web_msg.json_message, sizeof(web_msg.json_message));
        web_msg.len = strlen(web_msg.json_message);

        // Enviar a la cola para que WS lo mande a la web
        if (from_error_queue != NULL)
        {
            if (xQueueSend(from_error_queue, &web_msg, pdMS_TO_TICKS(100)) != pdTRUE)
            {
                ESP_LOGW(TAG, "No se pudo enviar error a from_error_queue (cola llena o timeout)");
            }
        }
    }

    // 3. Acciones especiales según el tipo de error
    switch (error->general_errors)
    {
    case SEND_CMD_QUEUE_ERROR:
        // Incrementar contador de comandos "perdidos"
        cmds_received++; // Se recibió pero no se pudo procesar
        evaluate_camera_state();
        break;

    case WEBSOCKET_DISCONNECTED:
        ESP_LOGW(TAG, "WebSocket desconectado - cámara puede seguir activa");
        break;

    case CAMERA_INIT_FAIL:
    case WEBSOCKET_INIT_FAIL:
    case WEBSOCKET_START_FAIL:
        ESP_LOGE(TAG, "Error crítico registrado: %s (id=%u)", error_name, error->id);
        break;

    default:
        break;
    }
}

/**
 * @brief Tarea principal de control de errores
 */
static void error_control_task(void *pvParameters)
{
    Error_inf received_error;

    ESP_LOGI(TAG, "Tarea error_control iniciada, esperando mensajes en to_error_queue");

    while (1)
    {
        // Esperar errores de cualquier módulo
        if (xQueueReceive(to_error_queue, &received_error, portMAX_DELAY) == pdTRUE)
        {
            process_error(&received_error);
        }
    }
}

/* ============== PUBLIC API ============== */

esp_err_t error_control_init(void)
{
    ESP_LOGI(TAG, "Inicializando módulo de control de errores");

    // Verificar que las colas existan
    if (to_error_queue == NULL)
    {
        ESP_LOGE(TAG, "error_control_init: to_error_queue es NULL (debe llamarse después de init_queues)");
        return ESP_FAIL;
    }

    if (from_error_queue == NULL)
    {
        ESP_LOGW(TAG, "error_control_init: from_error_queue es NULL, errores no se enviarán por WebSocket");
    }

    // Crear la tarea de control de errores
    BaseType_t result = xTaskCreate(
        error_control_task,
        "error_ctrl",
        ERROR_TASK_STACK_SIZE,
        NULL,
        ERROR_TASK_PRIORITY,
        &error_task_handle);

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "error_control_init: xTaskCreate error_ctrl FAIL");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Módulo de control de errores listo");
    return ESP_OK;
}

void error_control_stop(void)
{
    if (error_task_handle != NULL)
    {
        vTaskDelete(error_task_handle);
        error_task_handle = NULL;
        ESP_LOGI(TAG, "Control de errores detenido");
    }
}

void error_control_cmd_received(void)
{
    cmds_received++;
    evaluate_camera_state();
}

void error_control_cmd_sent_ok(void)
{
    cmds_sent_ok++;
    evaluate_camera_state();
}

camera_state_t error_control_get_camera_state(void)
{
    return current_camera_state;
}

void error_control_set_camera_state(camera_state_t state)
{
    if (state != current_camera_state)
    {
        ESP_LOGI(TAG, "Estado cámara cambiado manualmente a %d", (int)state);
        current_camera_state = state;
    }
}
