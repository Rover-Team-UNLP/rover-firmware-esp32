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
#include "esp_rom_uart.h"
#include "nvs_flash.h"

#include "app_globals.h"
#include "communication.h"
#include "wifi_manager.h"
#include "web_socket.h"
#include "error_control.h"
#include "uart_task.h"
#include "cam_task.h"

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
    // Tarea eliminada: no se loguean comandos por UART0
}

/**
 * @brief Inicializa las colas globales del sistema
 */
static esp_err_t init_queues(void)
{
    cmd_queue = xQueueCreate(10, sizeof(data_cmd));
    if (cmd_queue == NULL)
    {
        ESP_LOGE(TAG, "init_queues: cmd_queue create FAIL");
        return ESP_FAIL;
    }

    to_error_queue = xQueueCreate(10, sizeof(Error_inf));
    if (to_error_queue == NULL)
    {
        ESP_LOGE(TAG, "init_queues: to_error_queue create FAIL");
        return ESP_FAIL;
    }

    from_error_queue = xQueueCreate(10, sizeof(error_web_msg_t));
    if (from_error_queue == NULL)
    {
        ESP_LOGE(TAG, "init_queues: from_error_queue create FAIL");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void app_main(void)
{
    // Logs por UART0 para fase de integración (nivel INFO y superior)
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "========== Inicio firmware Rover ==========");

    // 1. Inicializar NVS (necesario para WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS: borrando y reinicializando (no free pages o nueva versión)");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS init OK");

    // 2. Inicializar las colas globales
    ret = init_queues();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "init_queues FAIL: no se pudieron crear las colas");
        return;
    }
    ESP_LOGI(TAG, "Colas globales creadas OK (cmd_queue, to_error_queue, from_error_queue)");

    // 3. Iniciar el módulo de control de errores
    ret = error_control_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "error_control_init FAIL: no se pudo crear la tarea de control de errores");
        return;
    }
    ESP_LOGI(TAG, "Control de errores init OK");

    // 4. Iniciar el módulo de WiFi Provisioning
    wifi_provisioning_init();
    ESP_LOGI(TAG, "WiFi provisioning iniciado, esperando conexión...");

    // 5. Esperar a que el WiFi se conecte
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "WiFi conectado");

        // 6. Iniciar el WebSocket
        ret = websocket_start();
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "WebSocket start OK");
        }
        else
        {
            ESP_LOGE(TAG, "WebSocket start FAIL: %s", esp_err_to_name(ret));
        }

        // 7. Iniciar UART (UART2, pines 12/13, comunicación CIAA)
        init_uart();
        xTaskCreate(task_uart, "uart_task", 4096, NULL, 5, NULL);
        ESP_LOGI(TAG, "UART2 init OK, tarea uart_task creada");

        // 8. Iniciar Cámara
        ret = start_camera();
        if (ret == ESP_OK)
        {
            xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, NULL);
            ESP_LOGI(TAG, "Cámara init OK, tarea camera_task creada");
        }
        else
        {
            ESP_LOGE(TAG, "start_camera FAIL: %s", esp_err_to_name(ret));
        }

        ESP_LOGI(TAG, "========== Sistema listo ==========");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "WiFi FAIL: no se pudo conectar. Revisar credenciales y red.");
    }
    else
    {
        ESP_LOGE(TAG, "WiFi: evento inesperado (bits=0x%lx)", (unsigned long)bits);
    }
}