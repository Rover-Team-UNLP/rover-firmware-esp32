/* ======================================
 * File: json_parser.h
 * Description: JSON command parser and circular command buffer
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef JSON_PARSER
#define JSON_PARSER

#include "cJSON.h"
#include "stdint.h"
#include "string.h"
#include "communication.h"

#define CMD_BUFFER_LEN 10

typedef enum
{
    STATUS_OK = 0,
    STATUS_PARSE_ERROR,
    STATUS_ID_NOT_FOUND,
    STATUS_BUFFER_CMD_NULL,
    STATUS_NOT_VALID_ID
} json_parser_status_t;

typedef struct
{
    data_cmd buffer[CMD_BUFFER_LEN];
    uint16_t newest_id;
    uint8_t count;
} cmd_buffer_t;

extern cmd_buffer_t cmd_buffer;

/**
 * @brief Get a command from the buffer by id.
 * @param id Command id to look up.
 * @param command Output; filled with command data if found.
 * @return Status (STATUS_OK, STATUS_ID_NOT_FOUND, etc.).
 */
json_parser_status_t take_cmd(uint16_t id, data_cmd *command);

/**
 * @brief Modify a command in the buffer by id.
 * @param id Command id to modify.
 * @param command New command data.
 * @return Status code.
 */
json_parser_status_t modify_cmd(uint16_t id, data_cmd *command);

/**
 * @brief Parse JSON string into data_cmd and store in buffer.
 * @param data JSON string.
 * @param ret_id Output; id assigned to the stored command.
 * @return Status code.
 */
json_parser_status_t parse_json(char *data, uint16_t *ret_id);

/**
 * @brief Format a command as UART string (S:CMD:INTENSITY:ID:E).
 * @param cmd Command to format.
 * @param uart_string Output buffer.
 * @return Status code.
 */
json_parser_status_t parse_cmd(data_cmd cmd, char *uart_string);

#endif
