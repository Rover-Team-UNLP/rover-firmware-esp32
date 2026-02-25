/* ======================================
 * File: web_socket.c
 * Description: WebSocket client for Rover backend (commands and error stream)
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#include "web_socket.h"
#include "esp_crt_bundle.h"
#include "error_control.h"
#include "esp_log.h"
#include "server_ip_config.h"
#include "uart_task.h"
#include <stdio.h>

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void send_error_task(void *pvParameters);

#define WEBSOCKET_URI_MAX_LEN 64
static const int STACK_SIZE = 4096;

static esp_websocket_client_handle_t client_handler = NULL;
static TaskHandle_t task_handler = NULL;

esp_err_t websocket_start()
{
    static char uri_buf[WEBSOCKET_URI_MAX_LEN];
    char server_ip[MAX_SERVER_IP_LENGTH] = "192.168.1.34";

    if (storage_get_server_ip(server_ip, sizeof(server_ip)) != ESP_OK)
    {
        /* Keep default 192.168.1.34 */
    }
    (void)snprintf(uri_buf, sizeof(uri_buf), "ws://%s:8080/ws/esp", server_ip);

    esp_websocket_client_config_t config = {0};
    config.uri = uri_buf;
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

void websocket_trigger_reconnect(void)
{
    if (client_handler == NULL)
        return;
    esp_websocket_client_stop(client_handler);
    esp_websocket_client_start(client_handler);
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
        if (data->op_code != WS_TRANSPORT_OPCODES_TEXT)
        {
            return;
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
            ESP_LOGI("WS", "Command received via WebSocket: CMD=%u, INTENSITY=%u, ID=%u", new_cmd.cmd, new_cmd.intensity, new_cmd.id);
            if (xQueueSend(cmd_queue, &new_cmd, pdMS_TO_TICKS(100)) != pdTRUE)
            {
                websocket_handler_error.general_errors = SEND_CMD_QUEUE_ERROR;
                xQueueSend(to_error_queue, &websocket_handler_error, 0);
                free(buffer);
                return;
            }
            error_control_cmd_received();
            free(buffer);
        }
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        websocket_handler_error.general_errors = WEBSOCKET_DISCONNECTED;
        xQueueSend(to_error_queue, &websocket_handler_error, 0);
        uart_send_stop_to_ciaa();
        websocket_trigger_reconnect();
        break;
    default:
        break;
    }
}

static void send_error_task(void *pvParameters)
{
    error_web_msg_t error_msg;

    while (1)
    {
        if (xQueueReceive(from_error_queue, &error_msg, portMAX_DELAY) == pdTRUE)
        {
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