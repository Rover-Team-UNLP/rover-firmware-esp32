/* ======================================
- File: json_parser.h
- Description: Header of the json parser and command buffer handler.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2025-10-01
- ====================================== */

#ifndef JSON_PARSER
#define JSON_PARSER

#include "cJSON.h"
#include "stdint.h"
#include "string.h"

#define CMD_BUFFER_LEN 10
#define CMD_PARAMS_LEN 10

// We could define macros with the limits of the params. x_velocity_max, etc.

typedef enum
{
    CMD_MOVE_FORWARD = 0,
    CMD_MOVE_BACKWARDS,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT
} rover_cmd_type_t;

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
    uint16_t id;
    rover_cmd_type_t cmd;
    double params[CMD_PARAMS_LEN]; // This is a estimate, we should see if it's less or more.
    uint8_t total_params;

} data_cmd;

typedef struct
{
    data_cmd buffer[CMD_BUFFER_LEN];
    uint16_t newest_id;
    uint8_t count;
} cmd_buffer_t;

// Declaración externa del buffer
extern cmd_buffer_t cmd_buffer;

/**
 * @brief This function takes a command from the buffer using it's id.
 * @param id (uint16_t) The id of the command looked.
 * @param command (*data_cmd) The info of the command looked.
 * @return a status code.
 */
json_parser_status_t take_cmd(uint16_t id, data_cmd *command);

/**
 * @brief Let us modify a command using it's id.
 * @param id (uint16_t) The id of the command looked.
 * @return a status code.
 */
json_parser_status_t modify_cmd(uint16_t id, data_cmd *command);

/**
 * @brief Parse a JSON to a data_cmd struct and saves it in the buffer.
 * @param data (char *) a string with the json.
 * @return a status code.
 */
json_parser_status_t parse_json(char *data, char *uart_string);

#endif
