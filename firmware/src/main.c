/* ======================================
- File: main.c
- Description: Main application entry point for ESP32 Rover firmware
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-01-31
- ====================================== */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_globals.h"
#include "communication.h"
#include "wifi_manager.h"
#include "web_socket.h"

static const char *TAG = "MAIN";

// Definición de las colas globales
QueueHandle_t to_error_queue = NULL;
QueueHandle_t from_error_queue = NULL;
QueueHandle_t cmd_queue = NULL;

/**
 * @brief Convierte el tipo de comando a string legible
 */
static const char *cmd_type_to_string(rover_cmd_type_t cmd)
{
    switch (cmd)
    {
    case CMD_MOVE_FORWARD:
        return "MOVE_FORWARD";
    case CMD_MOVE_BACKWARDS:
        return "MOVE_BACKWARDS";
    case CMD_MOVE_LEFT:
        return "MOVE_LEFT";
    case CMD_MOVE_RIGHT:
        return "MOVE_RIGHT";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Tarea que lee de la cmd_queue y muestra los comandos por UART0 (logs)
 */
static void cmd_logger_task(void *pvParameters)
{
    data_cmd received_cmd;

    ESP_LOGI(TAG, "Command logger task started - waiting for commands...");

    while (1)
    {
        // Esperar indefinidamente por un comando en la cola
        if (xQueueReceive(cmd_queue, &received_cmd, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "========== COMMAND RECEIVED ==========");
            ESP_LOGI(TAG, "  ID: %u", received_cmd.id);
            ESP_LOGI(TAG, "  Command Type: %s (%d)",
                     cmd_type_to_string(received_cmd.cmd),
                     received_cmd.cmd);
            ESP_LOGI(TAG, "  Total Params: %u", received_cmd.total_params);

            // Mostrar los parámetros
            if (received_cmd.total_params > 0)
            {
                ESP_LOGI(TAG, "  Parameters:");
                for (uint8_t i = 0; i < received_cmd.total_params && i < CMD_PARAMS_LEN; i++)
                {
                    ESP_LOGI(TAG, "    [%d]: %.4f", i, received_cmd.params[i]);
                }
            }
            ESP_LOGI(TAG, "======================================");
        }
    }
}

/**
 * @brief Inicializa las colas globales del sistema
 */
static esp_err_t init_queues(void)
{
    cmd_queue = xQueueCreate(10, sizeof(data_cmd));
    if (cmd_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create cmd_queue");
        return ESP_FAIL;
    }

    to_error_queue = xQueueCreate(10, sizeof(Error_inf));
    if (to_error_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create to_error_queue");
        return ESP_FAIL;
    }

    from_error_queue = xQueueCreate(10, sizeof(Error_inf));
    if (from_error_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create from_error_queue");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "All queues initialized successfully");
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   ESP32 Rover Firmware Starting...");
    ESP_LOGI(TAG, "========================================");

    // 1. Inicializar NVS (necesario para WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized");

    // 2. Inicializar las colas globales
    ret = init_queues();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize queues, aborting...");
        return;
    }

    // 3. Iniciar el módulo de WiFi Provisioning
    ESP_LOGI(TAG, "Starting WiFi provisioning...");
    wifi_provisioning_init();

    // 4. Esperar a que el WiFi se conecte
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "   WiFi Connected Successfully!");
        ESP_LOGI(TAG, "========================================");

        // 5. Iniciar el WebSocket
        ESP_LOGI(TAG, "Starting WebSocket client...");
        ret = websocket_start();
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "WebSocket client started successfully");

            // 6. Crear tarea para loggear comandos recibidos
            xTaskCreate(
                cmd_logger_task,
                "cmd_logger",
                4096,
                NULL,
                5,
                NULL);
            ESP_LOGI(TAG, "Command logger task created");
            ESP_LOGI(TAG, "System ready - waiting for WebSocket commands...");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to start WebSocket client: %s", esp_err_to_name(ret));
        }
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "   WiFi Connection Failed!");
        ESP_LOGW(TAG, "   AP Mode Active - Connect to 'Rover_Setup'");
        ESP_LOGW(TAG, "   and configure WiFi credentials.");
        ESP_LOGW(TAG, "========================================");
    }
}