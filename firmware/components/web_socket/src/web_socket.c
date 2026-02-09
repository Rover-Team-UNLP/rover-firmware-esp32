/* ======================================
- File: web_socket.c
- Description: Implementation of the web socket logic for communication between ESP32-cam and WebServer.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */

#include "web_socket.h"
#include "esp_crt_bundle.h"
#include "error_control.h"
#include "esp_log.h"

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void send_error_task(void *pvParameters);

static const char *WEBSOCKET_URI = "ws://10.0.142.92:8080/ws/esp"; // Just for test, it should be another one.
static const int STACK_SIZE = 4096;

static esp_websocket_client_handle_t client_handler = NULL;
static TaskHandle_t task_handler = NULL;

esp_err_t websocket_start()
{

    esp_websocket_client_config_t config = {0};
    config.uri = WEBSOCKET_URI;
    config.reconnect_timeout_ms = 10000;
    // config.crt_bundle_attach = esp_crt_bundle_attach;
    // config.headers = "ngrok-skip-browser-warning: true\r\n";

    client_handler = esp_websocket_client_init(&config);

    if (!client_handler)
    {
        return ESP_FAIL;
    }

    esp_err_t err = esp_websocket_register_events(client_handler, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);

    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_websocket_client_start(client_handler);

    if (err != ESP_OK)
    {
        return err;
    }

    // Crear tarea para enviar errores a la web
    xTaskCreate(
        send_error_task,
        "ws_error_task",
        STACK_SIZE,
        NULL,
        5,
        &task_handler);

    return err;
}

void websocket_stop()
{
    if (task_handler != NULL)
    {
        vTaskDelete(task_handler);
        task_handler = NULL;
    }

    esp_websocket_client_stop(client_handler);
    esp_websocket_client_destroy(client_handler);
    client_handler = NULL;
}

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    Error_inf websocket_handler_error = {
        .id = 1,
        .source = WEB_SOCKET,
        .response_type = 1,
    };
    switch (event_id)
    {
    case WEBSOCKET_EVENT_DATA:
        // Ignorar PING/PONG y frames binarios - solo procesar TEXT
        if (data->op_code != WS_TRANSPORT_OPCODES_TEXT)
        {
            return; // Silenciosamente ignorar
        }
        if (data->data_len > 0)
        {
            char *buffer = malloc(data->data_len + 1);
            if (buffer == NULL)
            {
                websocket_handler_error.general_errors = NULL_BUFFER;
                xQueueSend(to_error_queue, &websocket_handler_error, 0);
                return;
            }
            memcpy(buffer, data->data_ptr, data->data_len);
            buffer[data->data_len] = '\0';
            uint16_t cmd_id = 0;
            json_parser_status_t parse_status = parse_json(buffer, &cmd_id);
            if (parse_status != STATUS_OK)
            {
                websocket_handler_error.general_errors = PARSE_ERROR;
                xQueueSend(to_error_queue, &websocket_handler_error, 0);
                free(buffer);
                return;
            }
            data_cmd new_cmd;
            parse_status = take_cmd(cmd_id, &new_cmd);
            if (parse_status != STATUS_OK)
            {
                websocket_handler_error.general_errors = PARSE_ERROR;
                xQueueSend(to_error_queue, &websocket_handler_error, 0);
                free(buffer);
                return;
            }
            ESP_LOGI("WS", "Comando recibido por WebSocket: CMD=%u, INTENSITY=%u, ID=%u", new_cmd.cmd, new_cmd.intensity, new_cmd.id);
            if (xQueueSend(cmd_queue, &new_cmd, pdMS_TO_TICKS(100)) != pdTRUE)
            {
                websocket_handler_error.general_errors = SEND_CMD_QUEUE_ERROR;
                xQueueSend(to_error_queue, &websocket_handler_error, 0);
                free(buffer);
                return;
            }
            // Notificar al control de errores que se recibió un comando
            error_control_cmd_received();
            free(buffer);
        }
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        websocket_handler_error.general_errors = WEBSOCKET_DISCONNECTED;
        xQueueSend(to_error_queue, &websocket_handler_error, 0);
        break;
    default:
        break;
    }
}

/**
 * @brief Tarea que recibe errores del módulo error_control y los envía por WebSocket
 */
static void send_error_task(void *pvParameters)
{
    error_web_msg_t error_msg;

    while (1)
    {
        // Esperar mensajes de error del módulo de control de errores
        if (xQueueReceive(from_error_queue, &error_msg, portMAX_DELAY) == pdTRUE)
        {
            // Solo enviar si el WebSocket está conectado
            if (esp_websocket_client_is_connected(client_handler))
            {
                esp_websocket_client_send_text(
                    client_handler,
                    error_msg.json_message,
                    error_msg.len,
                    pdMS_TO_TICKS(1000));
            }
        }
    }
}