/* ======================================
- File: cam_task.c
- Description: implementation of streaming of MJPEG capture with a Cam using a HTTP server
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */
#include "cam_task.h"
#include "esp_log.h"

static const char *TAG = "CAM_TASK";

static TaskHandle_t cam_task_handle = NULL;
static camera_state_t last_camera_state = CAM_STATE_NORMAL;

esp_err_t http_client_event_handler(esp_http_client_event_t *);

/**
 * @brief Ajusta la calidad de la cámara según el estado del sistema
 */
void camera_adjust_quality(camera_state_t state)
{
    if (state == last_camera_state)
    {
        return; // No hay cambio
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == NULL)
    {
        ESP_LOGW(TAG, "Cannot adjust camera - sensor not available");
        return;
    }

    switch (state)
    {
    case CAM_STATE_NORMAL:
        sensor->set_framesize(sensor, CAM_NORMAL_FRAMESIZE);
        sensor->set_quality(sensor, CAM_NORMAL_QUALITY);
        ESP_LOGI(TAG, "Camera set to NORMAL mode (QVGA, q=%d)", CAM_NORMAL_QUALITY);
        break;

    case CAM_STATE_DEGRADED:
        sensor->set_framesize(sensor, CAM_DEGRADED_FRAMESIZE);
        sensor->set_quality(sensor, CAM_DEGRADED_QUALITY);
        ESP_LOGW(TAG, "Camera set to DEGRADED mode (QQVGA, q=%d)", CAM_DEGRADED_QUALITY);
        break;

    case CAM_STATE_SUSPENDED:
        // No ajustamos aquí, la tarea se suspende en el loop principal
        ESP_LOGW(TAG, "Camera entering SUSPENDED mode");
        break;
    }

    last_camera_state = state;
}

/**
 * @brief Obtiene el delay entre frames según el estado actual
 */
static uint32_t get_frame_delay_ms(camera_state_t state)
{
    switch (state)
    {
    case CAM_STATE_DEGRADED:
        return CAM_DEGRADED_DELAY_MS;
    case CAM_STATE_NORMAL:
    default:
        return CAM_NORMAL_DELAY_MS;
    }
}

esp_err_t start_camera(void)
{
    camera_config_t config = {
        .pin_pwdn = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,
        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sccb_sda = SIOD_GPIO_NUM,
        .pin_sccb_scl = SIOC_GPIO_NUM,

        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,

        .xclk_freq_hz = 20000000, // 20 MHz XCLK
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QVGA, // 320x240 para empezar
        .jpeg_quality = 10,
        .fb_count = 2 // 2 buffers para evitar bloqueos
    };

    esp_err_t error = esp_camera_init(&config);

    return error;
}

void camera_task(void *pvParameters)
{
    cam_task_handle = xTaskGetCurrentTaskHandle();
    uint32_t notify_value;
    camera_state_t current_state;

    esp_http_client_config_t config = {
        .url = url, // Variable global o definida en header
        .method = HTTP_METHOD_POST,
        .event_handler = http_client_event_handler,
        .timeout_ms = 5000,
        .transport_type = HTTP_TRANSPORT_OVER_TCP};

    ESP_LOGI(TAG, "Camera task started");

    while (1)
    {
        // Verificar si la cámara debe estar suspendida
        current_state = error_control_get_camera_state();

        if (current_state == CAM_STATE_SUSPENDED)
        {
            ESP_LOGW(TAG, "Camera suspended - waiting for system to recover...");
            // Esperar hasta que el estado cambie
            while (error_control_get_camera_state() == CAM_STATE_SUSPENDED)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            ESP_LOGI(TAG, "Camera resuming operation");
            current_state = error_control_get_camera_state();
        }

        // Ajustar calidad según el estado
        camera_adjust_quality(current_state);

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "multipart/x-mixed-replace; boundary=frame");

        esp_err_t err = esp_http_client_open(client, -1);

        if (err != ESP_OK)
        {
            Error_inf error_msg = {.id = err, .source = CAMERA, .response_type = 0, .general_errors = HTTP_CLIENT_NO_OPEN};
            xQueueSend(to_error_queue, &error_msg, 0);

            esp_http_client_cleanup(client);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        BaseType_t notify_recv = xTaskNotifyWait(0, 0, &notify_value, pdMS_TO_TICKS(5000));
        bool connection_establish = (notify_recv == pdTRUE && notify_value == 1);

        while (connection_establish)
        {
            // Verificar estado de la cámara en cada iteración
            current_state = error_control_get_camera_state();

            // Si se suspende, salir del loop de streaming
            if (current_state == CAM_STATE_SUSPENDED)
            {
                ESP_LOGW(TAG, "Camera suspended during streaming - closing connection");
                connection_establish = false;
                break;
            }

            // Ajustar calidad si cambió el estado
            camera_adjust_quality(current_state);

            uint32_t ulStatus = 0;
            if (xTaskNotifyWait(0, 0, &ulStatus, 0) == pdTRUE)
            {
                if (ulStatus == 0)
                {
                    Error_inf error_msg = {.id = err, .source = CAMERA, .response_type = 0, .general_errors = UL_STATUS_FAIL};
                    xQueueSend(to_error_queue, &error_msg, 0);
                    connection_establish = false;
                    break;
                }
            }

            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb)
            {
                Error_inf error_msg = {.id = 2, .source = CAMERA, .response_type = 0, .general_errors = FRAME_NULL};
                xQueueSend(to_error_queue, &error_msg, 0);
                break;
            }

            char part_header[128];
            int header_len = snprintf(part_header, sizeof(part_header),
                                      "--frame\r\n"
                                      "Content-Type: image/jpeg\r\n"
                                      "Content-Length: %u\r\n\r\n",
                                      fb->len);

            bool write_err = false;
            // Secuencia de envío: Header -> Payload -> Footer
            if (esp_http_client_write(client, part_header, header_len) < 0)
                write_err = true;
            if (!write_err && esp_http_client_write(client, (const char *)fb->buf, fb->len) < 0)
                write_err = true;
            if (!write_err && esp_http_client_write(client, "\r\n", 2) < 0)
                write_err = true;

            esp_camera_fb_return(fb);

            if (write_err)
            {
                Error_inf error_msg = {.id = 3, .source = CAMERA, .response_type = 0, .general_errors = WRITE_ERROR};
                xQueueSend(to_error_queue, &error_msg, 0);
                connection_establish = false;
            }

            // Delay dinámico según el estado de la cámara
            vTaskDelay(pdMS_TO_TICKS(get_frame_delay_ms(current_state)));
        }

        esp_http_client_cleanup(client);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

esp_err_t http_client_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
        if (cam_task_handle != NULL)
        {
            xTaskNotify(cam_task_handle, 1, eSetValueWithOverwrite);
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        if (cam_task_handle != NULL)
        {
            xTaskNotify(cam_task_handle, 0, eSetValueWithOverwrite);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}