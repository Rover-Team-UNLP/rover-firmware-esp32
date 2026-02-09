/* ======================================
- File: uart_task.c
- Description: Implementation of the uart_task
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */
#include "uart_task.h"
#include "error_control.h"
#include "esp_log.h"
#include "esp_err.h"

typedef struct
{
    uart_resp_id_t response;
    uint16_t id;
} data_parse;

static data_parse parse_data(char *data, int);
static void ready_handler();
static void error_handler(data_parse);
static void not_known_response(void);

QueueHandle_t uart_queue;

void init_uart()
{

    uart_config_t config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY,
        .stop_bits = UART_STOP,
        .flow_ctrl = UART_FLOW,
        .rx_flow_ctrl_thresh = 122,

    };

    uart_param_config(UART_NUM, &config);

    uart_set_pin(UART_NUM, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    esp_err_t err = uart_driver_install(UART_NUM, RX_SIZE, TX_SIZE, QUEUE_SIZE, &uart_queue, 0);
    if (err == ESP_OK)
    {
        ESP_LOGI("UART_TASK", "UART2 driver OK (TX=GPIO%d RX=GPIO%d, %d baud)", UART_TX, UART_RX, UART_BAUD_RATE);
    }
    else
    {
        ESP_LOGE("UART_TASK", "uart_driver_install FAIL: %s", esp_err_to_name(err));
    }
}

void task_uart(void *pvParameters)
{
    while (1)
    {
        uart_event_t event;
        char data[RESPONSE_LEN + 1];
        data_parse ciaa_response;

        if (xQueueReceive(uart_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI("UART_TASK", "Evento UART recibido: tipo=%d, size=%d", event.type, event.size);
            if (event.type == UART_DATA)
            {
                int read_bytes = uart_read_bytes(UART_NUM, data, RESPONSE_LEN, pdMS_TO_TICKS(50));
                data[read_bytes] = '\0';
                ESP_LOGI("UART_TASK", "Datos recibidos por UART: '%s' (%d bytes)", data, read_bytes);
                ciaa_response = parse_data(data, read_bytes);
                if (ciaa_response.response != -1)
                {
                    ESP_LOGI("UART_TASK", "Respuesta parseada: response=%d, id=%d", ciaa_response.response, ciaa_response.id);
                    switch (ciaa_response.response)
                    {
                    case RESP_READY:
                        ready_handler();
                        break;
                    case RESP_ERR_INVALID_COMMAND:
                        ESP_LOGW("UART_TASK", "Comando inválido recibido");
                        error_handler(ciaa_response);
                        break;
                    case RESP_ERR_INVALID_PARAMS:
                        ESP_LOGW("UART_TASK", "Parámetros inválidos recibidos");
                        error_handler(ciaa_response);
                        break;
                    default:
                        ESP_LOGW("UART_TASK", "Respuesta desconocida recibida");
                        not_known_response();
                        break;
                    }
                }
                else
                {
                    ESP_LOGW("UART_TASK", "No se pudo parsear la respuesta recibida");
                }
            }
            // Puedes agregar más logs para otros tipos de eventos si lo deseas
        }
    }
}

static void ready_handler()
{
    ESP_LOGI("UART_TASK", "Se recibió READY, enviando comando a la CIAA");
    data_cmd command;
    char data[RESPONSE_LEN + 1];
    char cmd_str[CMD_LEN];

    BaseType_t is_cmd = xQueueReceive(cmd_queue, &command, 0);
    if (!is_cmd)
    {
        return;
    }

    // Formatear comando: S:CMD:INTENSITY:ID:E
    snprintf(cmd_str, CMD_LEN, cmd_format,
             (unsigned int)command.cmd,
             (unsigned int)command.intensity,
             (unsigned int)command.id);

    uint8_t i = 0;
    uint8_t is_ack = 0;

    while (i <= 3 && !is_ack)
    {
        ESP_LOGI("UART_TASK", "Enviando comando por UART: '%s' (intento %d)", cmd_str, i + 1);
        uart_write_bytes(UART_NUM, cmd_str, strlen(cmd_str));
        int read_bytes = uart_read_bytes(UART_NUM, data, RESPONSE_LEN, ACK_AWAIT_MS / portTICK_PERIOD_MS);
        data[read_bytes] = '\0';
        ESP_LOGI("UART_TASK", "Esperando ACK, recibido: '%s' (%d bytes)", data, read_bytes);
        if (parse_data(data, read_bytes).response == RESP_ACK)
        {
            is_ack = 1;
            ESP_LOGI("UART_TASK", "ACK recibido correctamente para el comando");
            // Notificar al control de errores que el comando se envió exitosamente
            error_control_cmd_sent_ok();
        }
        if (!is_ack && i < 3)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        i++;
    }
    if (!is_ack)
    {
        ESP_LOGE("UART_TASK", "No se recibió ACK tras 3 intentos");
    }

    if (!is_ack)
    {
        ESP_LOGW("UART_TASK", "NO_ACK tras %d intentos (cmd=%u id=%u)", i, (unsigned)command.cmd, (unsigned)command.id);
        Error_inf not_ack_error = {
            .id = command.id,
            .source = UART,
            .response_type = 0,
            .general_errors = NO_ACK,
        };

        xQueueSend(to_error_queue, &not_ack_error, 0);
    }
}

static void error_handler(data_parse error)
{
    rover_errors_t gen_err = UART_UNKNOWN_RESPONSE;
    if (error.response == RESP_ERR_INVALID_COMMAND)
        gen_err = UART_INVALID_CMD;
    else if (error.response == RESP_ERR_INVALID_PARAMS)
        gen_err = UART_INVALID_PARAMS;

    ESP_LOGE("UART_TASK", "Manejando error UART: response=%d, id=%d, tipo_error=%d", error.response, error.id, gen_err);
    Error_inf new_error = {
        .source = UART,
        .id = error.id,
        .response_type = error.response,
        .general_errors = gen_err,
    };
    xQueueSend(to_error_queue, &new_error, 0);
    // Aca deberíamos esperar un respuesta del manejador de errores, pero todavía tengo que diseñar las diferentes politics de error
}

static void not_known_response(void)
{
    ESP_LOGW("UART_TASK", "Respuesta desconocida recibida, notificando error");
    Error_inf unknown_error = {
        .id = 0,
        .source = UART,
        .response_type = 0,
        .general_errors = UART_UNKNOWN_RESPONSE,
    };
    xQueueSend(to_error_queue, &unknown_error, 0);
}

static data_parse parse_data(char *data, int read_bytes)
{
    data_parse final = {
        .id = -1,
        .response = -1};

    if (read_bytes <= 0 || read_bytes > RESPONSE_LEN)
    {
        return final;
    }

    if (*(data + read_bytes - 1) != 'E')
    {
        return final;
    }

    *(data + read_bytes) = '\0';

    uint8_t temp_resp;
    uint16_t temp_id;
    char start_char;
    char end_char;

    uint8_t items = sscanf(data, response_format, &start_char, &temp_resp, &temp_id, &end_char);

    if (items == 4 && start_char == 'S' && end_char == 'E')
    {
        final.id = temp_id;
        final.response = temp_resp;
    }

    return final;
}