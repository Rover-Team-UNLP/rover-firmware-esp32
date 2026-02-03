/* ======================================
- File: web_socket.c
- Description: Implementation of the web socket logic for communication between ESP32-cam and WebServer.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-01-30
- ====================================== */

#include "web_socket.h"
#include "esp_crt_bundle.h"

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
//void send_error_task (void * pvParameters);

static const char *WEBSOCKET_URI = "ws://192.168.0.24:8080/ws/esp"; // Just for test, it should be another one.
// static const int STACK_SIZE = 4096;

static esp_websocket_client_handle_t client_handler = NULL;
static TaskHandle_t task_handler = NULL;

esp_err_t websocket_start() {
    
    esp_websocket_client_config_t config = {0};
    config.uri = WEBSOCKET_URI;
    config.reconnect_timeout_ms =  10000;
    //config.crt_bundle_attach = esp_crt_bundle_attach;
    //config.headers = "ngrok-skip-browser-warning: true\r\n";

        client_handler = esp_websocket_client_init(&config);

    if (!client_handler) {
        return ESP_FAIL;
    }

    esp_err_t err = esp_websocket_register_events(client_handler,WEBSOCKET_EVENT_ANY,websocket_event_handler,NULL);
    
    if (err != ESP_OK) {
       return err;
    }

    err = esp_websocket_client_start(client_handler);

    if (err != ESP_OK) {
        return err;
    }

    /*xTaskCreate(
        send_error_task, 
        "ws_error_task", 
        STACK_SIZE,      
        NULL,            
        5,               
        &task_handler 
    );*/

    return err;
}

void websocket_stop() {
    if (task_handler != NULL)
    {
        vTaskDelete(task_handler);
        task_handler = NULL;
    }

    esp_websocket_client_stop(client_handler);
    esp_websocket_client_destroy(client_handler);
    client_handler = NULL;


}

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    Error_inf websocket_handler_error = {
        .id = 1, 
        .source = WEB_SOCKET,
        .response_type = 1,
    };
    switch (event_id)
    {
    case WEBSOCKET_EVENT_DATA:
        if (data->op_code == WS_TRANSPORT_OPCODES_TEXT && data->data_len > 0) {
            char * buffer = malloc(data->data_len + 1);
            if (buffer == NULL) {
                websocket_handler_error.general_errors = NULL_BUFFER;
                xQueueSend(to_error_queue,&websocket_handler_error,0);
                return;
            }
            memcpy(buffer,data->data_ptr,data->data_len);
            buffer[data->data_len] = '\0';
            uint16_t cmd_id = 0; 
            json_parser_status_t parse_status = parse_json(buffer,&cmd_id);
            if (parse_status != STATUS_OK) {
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
            if (xQueueSend(cmd_queue, &new_cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
                websocket_handler_error.general_errors = SEND_CMD_QUEUE_ERROR;
                xQueueSend(to_error_queue, &websocket_handler_error, 0);
                free(buffer);
                return; 
            }
            free(buffer);
        } else  {
            websocket_handler_error.general_errors = OP_CODE_ERROR;
            xQueueSend(to_error_queue, &websocket_handler_error, 0);
            return;
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

/* To-do
void send_error_task(void *pvParameters) {
    
    rover_error_t error; 
    xQueueReceive(from_error_queue,&error,portMAX_DELAY);
    if (esp_websocket_client_is_connected(client_handler)) {
        Aca podríamos reenviar el error o enviar otro. 
        return;
    }
    lógica de conversion de error a json.
    esp_websocket_client_send_text(client_handle, json_string, len, timeout);
} */