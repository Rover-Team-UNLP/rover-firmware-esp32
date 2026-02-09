/* ======================================
- File: cam_task.c
- Description: implementation of streaming of MJPEG capture with a Cam using a HTTP server
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-02-03
- ====================================== */
#include "cam_task.h"
#include "esp_log.h"

static const char *TAG = "CAM_TASK";

// URL del servidor para streaming
const char *CAM_SERVER_URL = "http://10.0.142.92:8080/video/upload";

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
        .frame_size = CAM_NORMAL_FRAMESIZE,
        .jpeg_quality = CAM_JPEG_QUALITY_INIT,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_DRAM, // Usar memoria interna
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY};

    esp_err_t error = esp_camera_init(&config);

    return error;
}

void camera_task(void *pvParameters)
{
    cam_task_handle = xTaskGetCurrentTaskHandle();
    camera_state_t current_state;

    ESP_LOGI(TAG, "Camera task started");

    while (1)
    {
        // Verificar si la cámara debe estar suspendida
        current_state = error_control_get_camera_state();

        if (current_state == CAM_STATE_SUSPENDED)
        {
            ESP_LOGW(TAG, "Camera suspended - waiting for system to recover...");
            while (error_control_get_camera_state() == CAM_STATE_SUSPENDED)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            ESP_LOGI(TAG, "Camera resuming operation");
            current_state = error_control_get_camera_state();
        }

        // Ajustar calidad según el estado
        camera_adjust_quality(current_state);

        // Capturar frame
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGW(TAG, "Failed to capture frame");
            Error_inf error_msg = {.id = 2, .source = CAMERA, .response_type = 0, .general_errors = FRAME_NULL};
            xQueueSend(to_error_queue, &error_msg, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // Construir el body multipart para un solo frame
        const char *boundary = "frame";
        char part_header[256];
        int header_len = snprintf(part_header, sizeof(part_header),
                                  "--%s\r\n"
                                  "Content-Type: image/jpeg\r\n"
                                  "Content-Length: %zu\r\n\r\n",
                                  boundary, fb->len);

        char part_footer[32];
        int footer_len = snprintf(part_footer, sizeof(part_footer), "\r\n--%s--\r\n", boundary);

        int total_len = header_len + fb->len + footer_len;

        // Configurar cliente HTTP
        esp_http_client_config_t config = {
            .url = CAM_SERVER_URL,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 5000,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "multipart/x-mixed-replace; boundary=frame");

        // Abrir con Content-Length específico
        esp_err_t err = esp_http_client_open(client, total_len);

        if (err == ESP_OK)
        {
            // Enviar: part_header + jpeg + part_footer
            int written = esp_http_client_write(client, part_header, header_len);
            if (written >= 0)
            {
                written = esp_http_client_write(client, (const char *)fb->buf, fb->len);
            }
            if (written >= 0)
            {
                written = esp_http_client_write(client, part_footer, footer_len);
            }

            if (written < 0)
            {
                Error_inf error_msg = {.id = 3, .source = CAMERA, .response_type = 0, .general_errors = WRITE_ERROR};
                xQueueSend(to_error_queue, &error_msg, 0);
            }
            else
            {
                // Leer respuesta
                esp_http_client_fetch_headers(client);
                int status = esp_http_client_get_status_code(client);
                if (status != 200 && status != 201 && status != 204)
                {
                    ESP_LOGW(TAG, "Server returned status: %d", status);
                }
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to open HTTP: %s", esp_err_to_name(err));
            Error_inf error_msg = {.id = err, .source = CAMERA, .response_type = 0, .general_errors = HTTP_CLIENT_NO_OPEN};
            xQueueSend(to_error_queue, &error_msg, 0);
        }

        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        esp_camera_fb_return(fb);

        // Delay según estado
        vTaskDelay(pdMS_TO_TICKS(get_frame_delay_ms(current_state)));
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