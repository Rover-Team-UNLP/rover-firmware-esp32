/* ======================================
 * File: json_parser.c
 * Description: JSON command parser and circular command buffer
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */
#include <stdio.h>
#include "json_parser.h"

static json_parser_status_t push(const data_cmd *cmd);

cmd_buffer_t cmd_buffer = {0};

json_parser_status_t parse_json(char *data, uint16_t *ret_id)
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
    *ret_id = new_command.id;

    cJSON *intensity = cJSON_GetObjectItem(json, "intensity");
    if (!(cJSON_IsNumber(intensity)))
    {
        cJSON_Delete(json);
        return STATUS_PARSE_ERROR;
    }
    new_command.intensity = (uint8_t)intensity->valueint;

    push(&new_command);
    cJSON_Delete(json);
    return STATUS_OK;
}

json_parser_status_t parse_cmd(data_cmd cmd, char *uart_string)
{
    snprintf(uart_string, CMD_LEN, "S:%u:%u:%u:E",
             (unsigned int)cmd.cmd,
             (unsigned int)cmd.intensity,
             (unsigned int)cmd.id);
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