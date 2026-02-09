#include <unity.h>
#include <string.h>
#include "json_parser.h"
#include <stdio.h>

void setUp(void)
{
    memset(&cmd_buffer, 0, sizeof(cmd_buffer_t));
}

void tearDown(void)
{
}

// Test básico de parsing JSON válido con intensity
void test_parse_json_valid_basic(void)
{
    char *data = "{\"id\": 1, \"cmd\": 0, \"intensity\": 150}";
    uint16_t ret_id = 0;

    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data, &ret_id));
    TEST_ASSERT_EQUAL(1, ret_id);

    // Verificar que se guardó correctamente en el buffer
    data_cmd retrieved_cmd = {0};
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(1, &retrieved_cmd));
    TEST_ASSERT_EQUAL(1, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_FORWARD, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(150, retrieved_cmd.intensity);
}

// Test con diferentes tipos de comando
void test_parse_json_different_commands(void)
{
    uint16_t ret_id = 0;
    data_cmd retrieved_cmd = {0};

    // CMD_MOVE_FORWARD = 0
    char *data1 = "{\"id\": 1, \"cmd\": 0, \"intensity\": 100}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data1, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(1, &retrieved_cmd));
    TEST_ASSERT_EQUAL(CMD_MOVE_FORWARD, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(100, retrieved_cmd.intensity);

    // CMD_MOVE_BACKWARDS = 1
    char *data2 = "{\"id\": 2, \"cmd\": 1, \"intensity\": 200}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data2, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(2, &retrieved_cmd));
    TEST_ASSERT_EQUAL(CMD_MOVE_BACKWARDS, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(200, retrieved_cmd.intensity);

    // CMD_MOVE_LEFT = 2
    char *data3 = "{\"id\": 3, \"cmd\": 2, \"intensity\": 50}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data3, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(3, &retrieved_cmd));
    TEST_ASSERT_EQUAL(CMD_MOVE_LEFT, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(50, retrieved_cmd.intensity);

    // CMD_MOVE_RIGHT = 3
    char *data4 = "{\"id\": 4, \"cmd\": 3, \"intensity\": 255}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data4, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(4, &retrieved_cmd));
    TEST_ASSERT_EQUAL(CMD_MOVE_RIGHT, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(255, retrieved_cmd.intensity);
}

// Test valores límite de intensity (0-255)
void test_parse_json_intensity_limits(void)
{
    uint16_t ret_id = 0;
    data_cmd retrieved_cmd = {0};

    // Intensity mínimo (0)
    char *data1 = "{\"id\": 10, \"cmd\": 0, \"intensity\": 0}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data1, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(10, &retrieved_cmd));
    TEST_ASSERT_EQUAL(0, retrieved_cmd.intensity);

    // Intensity máximo (255)
    char *data2 = "{\"id\": 11, \"cmd\": 1, \"intensity\": 255}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data2, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(11, &retrieved_cmd));
    TEST_ASSERT_EQUAL(255, retrieved_cmd.intensity);

    // Intensity medio
    char *data3 = "{\"id\": 12, \"cmd\": 2, \"intensity\": 128}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data3, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(12, &retrieved_cmd));
    TEST_ASSERT_EQUAL(128, retrieved_cmd.intensity);
}

// Test de errores de parsing
void test_parse_json_invalid_cases(void)
{
    uint16_t ret_id = 0;

    // JSON malformado
    char *invalid_json = "{\"id\": 1, \"cmd\": 0 \"intensity\": 100}"; // falta coma
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(invalid_json, &ret_id));

    // ID faltante
    char *no_id = "{\"cmd\": 0, \"intensity\": 100}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(no_id, &ret_id));

    // CMD faltante
    char *no_cmd = "{\"id\": 1, \"intensity\": 100}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(no_cmd, &ret_id));

    // Intensity faltante
    char *no_intensity = "{\"id\": 1, \"cmd\": 0}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(no_intensity, &ret_id));

    // ID no es número
    char *id_string = "{\"id\": \"abc\", \"cmd\": 0, \"intensity\": 100}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(id_string, &ret_id));

    // CMD no es número
    char *cmd_string = "{\"id\": 1, \"cmd\": \"forward\", \"intensity\": 100}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(cmd_string, &ret_id));

    // Intensity no es número
    char *intensity_string = "{\"id\": 1, \"cmd\": 0, \"intensity\": \"max\"}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(intensity_string, &ret_id));
}

// Test función take_cmd
void test_take_cmd_functionality(void)
{
    uint16_t ret_id = 0;

    // Primero insertamos algunos comandos
    char *data1 = "{\"id\": 5, \"cmd\": 1, \"intensity\": 100}";
    char *data2 = "{\"id\": 7, \"cmd\": 2, \"intensity\": 200}";

    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data1, &ret_id));
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data2, &ret_id));

    // Ahora probamos take_cmd
    data_cmd retrieved_cmd = {0};

    // Buscar comando con ID 5
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(5, &retrieved_cmd));
    TEST_ASSERT_EQUAL(5, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_BACKWARDS, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(100, retrieved_cmd.intensity);

    // Buscar comando con ID 7
    memset(&retrieved_cmd, 0, sizeof(data_cmd));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(7, &retrieved_cmd));
    TEST_ASSERT_EQUAL(7, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_LEFT, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(200, retrieved_cmd.intensity);
}

// Test errores de take_cmd
void test_take_cmd_errors(void)
{
    data_cmd retrieved_cmd = {0};

    // ID inválido (0)
    TEST_ASSERT_EQUAL(STATUS_NOT_VALID_ID, take_cmd(0, &retrieved_cmd));

    // ID que no existe en el buffer
    TEST_ASSERT_EQUAL(STATUS_ID_NOT_FOUND, take_cmd(999, &retrieved_cmd));
}

// Test función modify_cmd
void test_modify_cmd_functionality(void)
{
    uint16_t ret_id = 0;

    // Primero insertar un comando
    char *data = "{\"id\": 8, \"cmd\": 0, \"intensity\": 50}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data, &ret_id));

    // Modificar el comando
    data_cmd new_cmd = {
        .id = 8,
        .cmd = CMD_MOVE_RIGHT,
        .intensity = 200};

    TEST_ASSERT_EQUAL(STATUS_OK, modify_cmd(8, &new_cmd));

    // Verificar que se modificó correctamente
    data_cmd retrieved_cmd = {0};
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(8, &retrieved_cmd));
    TEST_ASSERT_EQUAL(8, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_RIGHT, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(200, retrieved_cmd.intensity);
}

// Test errores de modify_cmd
void test_modify_cmd_errors(void)
{
    data_cmd test_cmd = {
        .id = 1,
        .cmd = CMD_MOVE_FORWARD,
        .intensity = 100};

    // ID inválido (0)
    TEST_ASSERT_EQUAL(STATUS_NOT_VALID_ID, modify_cmd(0, &test_cmd));

    // Puntero NULL
    TEST_ASSERT_EQUAL(STATUS_NOT_VALID_ID, modify_cmd(1, NULL));

    // ID que no existe
    TEST_ASSERT_EQUAL(STATUS_ID_NOT_FOUND, modify_cmd(999, &test_cmd));
}

// Test buffer circular (overflow)
void test_buffer_circular_behavior(void)
{
    uint16_t ret_id = 0;

    // Llenar el buffer más allá de su capacidad (CMD_BUFFER_LEN = 10)
    for (int i = 1; i <= 15; i++)
    {
        char json_data[100];
        snprintf(json_data, sizeof(json_data),
                 "{\"id\": %d, \"cmd\": 0, \"intensity\": %d}", i, i * 10);
        TEST_ASSERT_EQUAL(STATUS_OK, parse_json(json_data, &ret_id));
    }

    // Los primeros 5 comandos deberían haber sido sobrescritos
    data_cmd retrieved_cmd = {0};

    // ID 1-5 deberían haber sido sobrescritos por 11-15
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(11, &retrieved_cmd));
    TEST_ASSERT_EQUAL(11, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(110, retrieved_cmd.intensity);

    // ID 6-10 deberían seguir existiendo
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(6, &retrieved_cmd));
    TEST_ASSERT_EQUAL(6, retrieved_cmd.id);

    // ID 15 debería estar en la posición del ID 5 original
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(15, &retrieved_cmd));
    TEST_ASSERT_EQUAL(15, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(150, retrieved_cmd.intensity);
}

// Test parse_cmd - genera string UART formato S:CMD:INTENSITY:ID:E
void test_parse_cmd_format(void)
{
    char uart_string[CMD_LEN] = {0};

    data_cmd cmd1 = {
        .id = 1,
        .cmd = CMD_MOVE_FORWARD,
        .intensity = 100};
    TEST_ASSERT_EQUAL(STATUS_OK, parse_cmd(cmd1, uart_string));
    TEST_ASSERT_EQUAL_STRING("S:0:100:1:E", uart_string);

    data_cmd cmd2 = {
        .id = 42,
        .cmd = CMD_MOVE_BACKWARDS,
        .intensity = 255};
    memset(uart_string, 0, CMD_LEN);
    TEST_ASSERT_EQUAL(STATUS_OK, parse_cmd(cmd2, uart_string));
    TEST_ASSERT_EQUAL_STRING("S:1:255:42:E", uart_string);

    data_cmd cmd3 = {
        .id = 100,
        .cmd = CMD_MOVE_LEFT,
        .intensity = 0};
    memset(uart_string, 0, CMD_LEN);
    TEST_ASSERT_EQUAL(STATUS_OK, parse_cmd(cmd3, uart_string));
    TEST_ASSERT_EQUAL_STRING("S:2:0:100:E", uart_string);

    data_cmd cmd4 = {
        .id = 7,
        .cmd = CMD_MOVE_RIGHT,
        .intensity = 128};
    memset(uart_string, 0, CMD_LEN);
    TEST_ASSERT_EQUAL(STATUS_OK, parse_cmd(cmd4, uart_string));
    TEST_ASSERT_EQUAL_STRING("S:3:128:7:E", uart_string);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_json_valid_basic);
    RUN_TEST(test_parse_json_different_commands);
    RUN_TEST(test_parse_json_intensity_limits);
    RUN_TEST(test_parse_json_invalid_cases);
    RUN_TEST(test_take_cmd_functionality);
    RUN_TEST(test_take_cmd_errors);
    RUN_TEST(test_modify_cmd_functionality);
    RUN_TEST(test_modify_cmd_errors);
    RUN_TEST(test_buffer_circular_behavior);
    RUN_TEST(test_parse_cmd_format);

    UNITY_END();

    return 0;
}