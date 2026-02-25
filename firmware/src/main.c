/* ======================================
 * File: main.c
 * Description: Main application entry point for ESP32 Rover firmware
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

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
#include "nvs_storage.h"
#include "web_socket.h"
#include "error_control.h"
#include "uart_task.h"
#include "cam_task.h"

static const char *TAG = "MAIN";

QueueHandle_t to_error_queue = NULL;
QueueHandle_t from_error_queue = NULL;
QueueHandle_t cmd_queue = NULL;

/** Convert command type to string for logging. */
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
    case CMD_STOP:
        return "STOP";
    default:
        return "UNKNOWN";
    }
}

/** Placeholder task (command logging disabled). */
static void cmd_logger_task(void *pvParameters)
{
    (void)pvParameters;
}

/** Create global queues (cmd_queue, to_error_queue, from_error_queue). */
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
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("cam_hal", ESP_LOG_NONE);

    ESP_LOGI(TAG, "========== Rover firmware start ==========");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS: erase and reinit (no free pages or new version)");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS init OK");

    ret = init_queues();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "init_queues FAIL");
        return;
    }
    ESP_LOGI(TAG, "Queues created OK");

    ret = error_control_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "error_control_init FAIL");
        return;
    }
    ESP_LOGI(TAG, "Error control init OK");

    wifi_provisioning_init();
    ESP_LOGI(TAG, "WiFi provisioning started, waiting for connection...");

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "WiFi connected");

        char server_ip[MAX_SERVER_IP_LENGTH] = "192.168.1.34";
        (void)storage_get_server_ip(server_ip, sizeof(server_ip));
        char cam_url_buf[64];
        snprintf(cam_url_buf, sizeof(cam_url_buf), "http://%s:8080/video/upload", server_ip);
        cam_task_set_server_url(cam_url_buf);

        ret = websocket_start();
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "WebSocket start OK");
        }
        else
        {
            ESP_LOGE(TAG, "WebSocket start FAIL: %s", esp_err_to_name(ret));
        }

        init_uart();
        xTaskCreate(task_uart, "uart_task", 4096, NULL, 5, NULL);
        ESP_LOGI(TAG, "UART2 init OK, uart_task created");

        ret = start_camera();
        if (ret == ESP_OK)
        {
            xTaskCreate(camera_task, "camera_task", 4096, NULL, 3, NULL);
            ESP_LOGI(TAG, "Camera init OK, camera_task created");
        }
        else
        {
            ESP_LOGE(TAG, "start_camera FAIL: %s", esp_err_to_name(ret));
        }

        ESP_LOGI(TAG, "========== System ready (server: %s) ==========", server_ip);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "WiFi FAIL: could not connect. Check credentials and network.");
    }
    else
    {
        ESP_LOGE(TAG, "WiFi: unexpected event (bits=0x%lx)", (unsigned long)bits);
    }
}