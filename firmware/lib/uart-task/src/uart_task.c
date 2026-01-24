/* ======================================
- File: uart_task.c
- Description: Implementation of the uart_task
- Author/s: @JuanCruzFerreiraM
- Last-update: 2025-10-20
- ====================================== */
#include "uart_task.h"

typedef struct
{
    uart_resp_id_t response;
    uint16_t id;
} data_parse;

data_parse parse_data(char *data, int);
void ready_handler();
void error_handler(data_parse);
void not_known_response();

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

    uart_driver_install(UART_NUM, RX_SIZE, TX_SIZE, QUEUE_SIZE, &uart_queue, 0);
}

void task_uart()
{
    while (1)
    {
        uart_event_t event;
        uint8_t is_data = 0;
        char data[RESPONSE_LEN + 1];
        data_parse ciaa_response;

        if (xQueueReceive(uart_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            if (event.type == UART_DATA)
            {
                int read_bytes = uart_read_bytes(UART_NUM, data, RESPONSE_LEN, pdMS_TO_TICKS(50));
                ciaa_response = parse_data(data, read_bytes);
                if (ciaa_response.response != -1)
                {
                    switch (ciaa_response.response)
                    {
                    case RESP_READY:
                        ready_handler();
                        break;
                    case RESP_ERR_INVALID_COMMAND:
                        error_handler(ciaa_response);
                        break;
                    case RESP_ERR_INVALID_PARAMS:
                        error_handler(ciaa_response);
                        break;
                    default:
                        not_known_response();
                        break;
                    }
                }
            }
        }
    }
}

void ready_handler()
{
    char *command;
    char data[RESPONSE_LEN + 1];
    BaseType_t is_cmd = xQueueReceive(cmd_queue, &command, 0);
    if (!is_cmd)
    {
        return;
    }

    uint8_t i = 0;
    uint8_t is_ack = 0;

    while (i <= 3 && !is_ack)
    {
        uart_write_bytes(UART_NUM, (const char *)command, strlen(command));
        int read_bytes = uart_read_bytes(UART_NUM, data, RESPONSE_LEN, ACK_AWAIT_MS / portTICK_PERIOD_MS);
        if (parse_data(data, read_bytes).response == RESP_ACK)
        {
            is_ack = 1;
        }
        if (!is_ack && i < 3)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        i++;
    }
    vPortFree(command);
    if (!is_ack)
    {
        // Debemos agregar el manejo internto de errores, colocando un error en la cola de errores, agregarlo cuando modifiquemos eso.
    }
}

void error_handler(data_parse error) {
    Error_inf new_error = {
        .source = UART,
        .id = error.id,
        .response_type = error.response,
    };
    xQueueSend(error_queue, &new_error, 0);
    //Aca deberíamos esperar un respuesta del manejador de errores, pero todavía tengo que diseñar las diferentes politics de error    
}

data_parse parse_data(char *data, int read_bytes)
{
    data_parse final = {
        .id = -1,
        .response = -1};

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