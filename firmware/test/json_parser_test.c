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

// Test básico de parsing JSON válido
void test_parse_json_valid_basic(void)
{
    char *data = "{\"id\": 1,\"cmd\": 0, \"params\": [1.50, 1.30]}";
    char uart_string[256] = {0};

    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data, uart_string));
    TEST_ASSERT_EQUAL_STRING("1-0-1.50-1.30\n", uart_string);
}

// Test con diferentes tipos de comando
void test_parse_json_different_commands(void)
{
    char uart_string[256] = {0};

    // CMD_MOVE_FORWARD = 0
    char *data1 = "{\"id\": 1,\"cmd\": 0, \"params\": [2.0]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data1, uart_string));
    TEST_ASSERT_EQUAL_STRING("1-0-2.00\n", uart_string);

    // CMD_MOVE_BACKWARDS = 1
    memset(uart_string, 0, 256);
    char *data2 = "{\"id\": 2,\"cmd\": 1, \"params\": [1.5, 2.5]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data2, uart_string));
    TEST_ASSERT_EQUAL_STRING("2-1-1.50-2.50\n", uart_string);

    // CMD_MOVE_LEFT = 2
    memset(uart_string, 0, 256);
    char *data3 = "{\"id\": 3,\"cmd\": 2, \"params\": [3.14]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data3, uart_string));
    TEST_ASSERT_EQUAL_STRING("3-2-3.14\n", uart_string);

    // CMD_MOVE_RIGHT = 3
    memset(uart_string, 0, 256);
    char *data4 = "{\"id\": 4,\"cmd\": 3, \"params\": [0.75, 1.25, 2.75]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data4, uart_string));
    TEST_ASSERT_EQUAL_STRING("4-3-0.75-1.25-2.75\n", uart_string);
}

// Test con arrays de parámetros de diferentes tamaños
void test_parse_json_different_param_sizes(void)
{
    char uart_string[256] = {0};

    // Sin parámetros
    char *data1 = "{\"id\": 10,\"cmd\": 0, \"params\": []}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data1, uart_string));
    TEST_ASSERT_EQUAL_STRING("10-0\n", uart_string);

    // Un parámetro
    memset(uart_string, 0, 256);
    char *data2 = "{\"id\": 11,\"cmd\": 1, \"params\": [5.0]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data2, uart_string));
    TEST_ASSERT_EQUAL_STRING("11-1-5.00\n", uart_string);

    // Máximo número de parámetros (CMD_PARAMS_LEN = 10)
    memset(uart_string, 0, 256);
    char *data3 = "{\"id\": 12,\"cmd\": 2, \"params\": [1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data3, uart_string));
    TEST_ASSERT_EQUAL_STRING("12-2-1.00-2.00-3.00-4.00-5.00-6.00-7.00-8.00-9.00-10.00\n", uart_string);
}

// Test de errores de parsing
void test_parse_json_invalid_cases(void)
{
    char uart_string[256] = {0};

    // JSON malformado
    char *invalid_json = "{\"id\": 1,\"cmd\": 0 \"params\": [1.0]}"; // falta coma
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(invalid_json, uart_string));

    // ID faltante
    char *no_id = "{\"cmd\": 0, \"params\": [1.0]}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(no_id, uart_string));

    // CMD faltante
    char *no_cmd = "{\"id\": 1, \"params\": [1.0]}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(no_cmd, uart_string));

    // Params faltante
    char *no_params = "{\"id\": 1, \"cmd\": 0}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(no_params, uart_string));

    // ID no es número
    char *id_string = "{\"id\": \"abc\",\"cmd\": 0, \"params\": [1.0]}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(id_string, uart_string));

    // CMD no es número
    char *cmd_string = "{\"id\": 1,\"cmd\": \"forward\", \"params\": [1.0]}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(cmd_string, uart_string));

    // Params no es array
    char *params_not_array = "{\"id\": 1,\"cmd\": 0, \"params\": \"1.0,2.0\"}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(params_not_array, uart_string));

    // Demasiados parámetros (más de CMD_PARAMS_LEN = 10)
    char *too_many_params = "{\"id\": 1,\"cmd\": 0, \"params\": [1,2,3,4,5,6,7,8,9,10,11]}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(too_many_params, uart_string));

    // Parámetro no es número
    char *param_not_number = "{\"id\": 1,\"cmd\": 0, \"params\": [1.0, \"abc\"]}";
    TEST_ASSERT_EQUAL(STATUS_PARSE_ERROR, parse_json(param_not_number, uart_string));
}

// Test función take_cmd
void test_take_cmd_functionality(void)
{
    // Primero insertamos algunos comandos
    char uart_string[256] = {0};
    char *data1 = "{\"id\": 5,\"cmd\": 1, \"params\": [1.0, 2.0]}";
    char *data2 = "{\"id\": 7,\"cmd\": 2, \"params\": [3.0]}";

    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data1, uart_string));
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data2, uart_string));

    // Ahora probamos take_cmd
    data_cmd retrieved_cmd = {0};

    // Buscar comando con ID 5
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(5, &retrieved_cmd));
    TEST_ASSERT_EQUAL(5, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_BACKWARDS, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(2, retrieved_cmd.total_params);
    TEST_ASSERT_EQUAL_FLOAT(1.0, retrieved_cmd.params[0]);
    TEST_ASSERT_EQUAL_FLOAT(2.0, retrieved_cmd.params[1]);

    // Buscar comando con ID 7
    memset(&retrieved_cmd, 0, sizeof(data_cmd));
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(7, &retrieved_cmd));
    TEST_ASSERT_EQUAL(7, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_LEFT, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(1, retrieved_cmd.total_params);
    TEST_ASSERT_EQUAL_FLOAT(3.0, retrieved_cmd.params[0]);
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
    // Primero insertar un comando
    char uart_string[256] = {0};
    char *data = "{\"id\": 8,\"cmd\": 0, \"params\": [1.0]}";
    TEST_ASSERT_EQUAL(STATUS_OK, parse_json(data, uart_string));

    // Modificar el comando
    data_cmd new_cmd = {
        .id = 8,
        .cmd = CMD_MOVE_RIGHT,
        .params = {5.5, 6.6},
        .total_params = 2};

    TEST_ASSERT_EQUAL(STATUS_OK, modify_cmd(8, &new_cmd));

    // Verificar que se modificó correctamente
    data_cmd retrieved_cmd = {0};
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(8, &retrieved_cmd));
    TEST_ASSERT_EQUAL(8, retrieved_cmd.id);
    TEST_ASSERT_EQUAL(CMD_MOVE_RIGHT, retrieved_cmd.cmd);
    TEST_ASSERT_EQUAL(2, retrieved_cmd.total_params);
    TEST_ASSERT_EQUAL_FLOAT(5.5, retrieved_cmd.params[0]);
    TEST_ASSERT_EQUAL_FLOAT(6.6, retrieved_cmd.params[1]);
}

// Test errores de modify_cmd
void test_modify_cmd_errors(void)
{
    data_cmd test_cmd = {
        .id = 1,
        .cmd = CMD_MOVE_FORWARD,
        .params = {1.0},
        .total_params = 1};

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
    char uart_string[256] = {0};

    // Llenar el buffer más allá de su capacidad (CMD_BUFFER_LEN = 10)
    for (int i = 1; i <= 15; i++)
    {
        char json_data[100];
        snprintf(json_data, sizeof(json_data),
                 "{\"id\": %d,\"cmd\": 0, \"params\": [%d.0]}", i, i);
        TEST_ASSERT_EQUAL(STATUS_OK, parse_json(json_data, uart_string));
    }

    // Los primeros 5 comandos deberían haber sido sobrescritos
    data_cmd retrieved_cmd = {0};

    // ID 1-5 deberían haber sido sobrescritos por 11-15
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(11, &retrieved_cmd));
    TEST_ASSERT_EQUAL(11, retrieved_cmd.id);

    // ID 6-10 deberían seguir existiendo
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(6, &retrieved_cmd));
    TEST_ASSERT_EQUAL(6, retrieved_cmd.id);

    // ID 15 debería estar en la posición del ID 5 original
    TEST_ASSERT_EQUAL(STATUS_OK, take_cmd(15, &retrieved_cmd));
    TEST_ASSERT_EQUAL(15, retrieved_cmd.id);
    TEST_ASSERT_EQUAL_FLOAT(15.0, retrieved_cmd.params[0]);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_json_valid_basic);
    RUN_TEST(test_parse_json_different_commands);
    RUN_TEST(test_parse_json_different_param_sizes);
    RUN_TEST(test_parse_json_invalid_cases);
    RUN_TEST(test_take_cmd_functionality);
    RUN_TEST(test_take_cmd_errors);
    RUN_TEST(test_modify_cmd_functionality);
    RUN_TEST(test_modify_cmd_errors);
    RUN_TEST(test_buffer_circular_behavior);

    UNITY_END();

    return 0;
}