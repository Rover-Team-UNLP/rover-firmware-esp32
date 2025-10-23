/* ======================================
- File: json_parser.c
- Description: JSON parser and command buffer handler.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2025-10-01
- ====================================== */
#include <stdio.h>
#include "json_parser.h"

/**
 * @brief Push a cmd to the cmd_buffer using a secuencial id logic.
 * @param cmd (data_cmd*) the command to insert in the buffer.
 * @return a status code.
 */
static json_parser_status_t push(const data_cmd *cmd);

cmd_buffer_t cmd_buffer = {0};

json_parser_status_t parse_json(char *data, char *uart_string)
{
    data_cmd new_command = {0};
    cJSON *json = cJSON_Parse(data);
    if (!json)
    {
        return STATUS_PARSE_ERROR;
    }
    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    if (!(cJSON_IsNumber(cmd)))
    {
        cJSON_Delete(json);
        return STATUS_PARSE_ERROR;
    }

    new_command.cmd = (rover_cmd_type_t)cmd->valueint;
    cJSON *id = cJSON_GetObjectItem(json, "id");
    if (!(cJSON_IsNumber(id)))
    {
        cJSON_Delete(json);
        return STATUS_PARSE_ERROR;
    }

    new_command.id = id->valueint;
    cJSON *params = cJSON_GetObjectItem(json, "params");
    if (!(cJSON_IsArray(params)))
    {
        cJSON_Delete(json);
        return STATUS_PARSE_ERROR;
    }

    int params_len = cJSON_GetArraySize(params);
    if (params_len > CMD_PARAMS_LEN)
    {
        cJSON_Delete(json);
        return STATUS_PARSE_ERROR;
    }
    snprintf(uart_string, 256, "%u-%d", new_command.id, new_command.cmd);
    if (params_len == 0) 
    {
        snprintf(uart_string + 4, 252, "\n");
    }
    for (int i = 0; i < params_len; i++)
    {
        cJSON *item = cJSON_GetArrayItem(params, i);
        if (!(cJSON_IsNumber(item)))
        {
            cJSON_Delete(json);
            return STATUS_PARSE_ERROR;
        }
        new_command.params[i] = item->valuedouble;
        size_t current_len = strlen(uart_string);
        size_t remaining = 256 - current_len;
        if (i == params_len - 1)
        {
            snprintf(uart_string + current_len, remaining, "-%.2f\n", new_command.params[i]);
        }
        else
        {
            snprintf(uart_string + current_len, remaining, "-%.2f", new_command.params[i]);
        }
    }
    new_command.total_params = params_len;
    push(&new_command);
    cJSON_Delete(json);
    return STATUS_OK;
}

json_parser_status_t take_cmd(uint16_t id, data_cmd *command)
{
    if (id == 0)
    {
        return STATUS_NOT_VALID_ID;
    }
    uint8_t index = (id - 1) % CMD_BUFFER_LEN;
    if (!(cmd_buffer.buffer[index].id == id))
    {
        return STATUS_ID_NOT_FOUND;
    }
    *command = cmd_buffer.buffer[index];
    return STATUS_OK;
}

json_parser_status_t modify_cmd(uint16_t id, data_cmd *command)
{
    if ((id == 0) || !command)
    {
        return STATUS_NOT_VALID_ID;
    }

    uint8_t index = (id - 1) % CMD_BUFFER_LEN;
    if (!(cmd_buffer.buffer[index].id == id))
    {
        return STATUS_ID_NOT_FOUND;
    }

    cmd_buffer.buffer[index] = *command;
    return STATUS_OK;
}

static json_parser_status_t push(const data_cmd *cmd)
{
    if (!cmd)
    {
        return STATUS_BUFFER_CMD_NULL;
    }
    uint8_t index = (cmd->id - 1) % CMD_BUFFER_LEN;
    cmd_buffer.buffer[index] = *cmd;
    cmd_buffer.newest_id = cmd->id;
    if (cmd_buffer.count < CMD_BUFFER_LEN)
    {
        cmd_buffer.count++;
    }

    return STATUS_OK;
}