/* ======================================
- File: cam_task.c
- Description: implementation of streaming of MJPEG capture with a Cam using a HTTP server
- Author/s: @JuanCruzFerreiraM
- Last-update: 2026-01-06
- ====================================== */
#include "cam_task.h"

static TaskHandle_t cam_task_handle = NULL;

esp_err_t http_client_event_handler(esp_http_client_event_t *);

esp_err_t start_camera()
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
    esp_http_client_config_t config = {
        .url = url, // Variable global o definida en header
        .method = HTTP_METHOD_POST,
        .event_handler = http_client_event_handler,
        .timeout_ms = 5000,
        .transport_type = HTTP_TRANSPORT_OVER_TCP};

    while (1)
    {

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "multipart/x-mixed-replace; boundary=frame");

        esp_err_t err = esp_http_client_open(client, -1);

        if (err != ESP_OK)
        {
            Error_inf error_msg = {.id = err, .source = CAMERA, .response_type = 0};
            xQueueSend(error_queue, &error_msg, 0);

            esp_http_client_cleanup(client);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        BaseType_t notify_recv = xTaskNotifyWait(0, 0, &notify_value, pdMS_TO_TICKS(5000));
        bool connection_establish = (notify_recv == pdTRUE && notify_value == 1);

        while (connection_establish)
        {
            uint32_t ulStatus = 0;
            if (xTaskNotifyWait(0, 0, &ulStatus, 0) == pdTRUE)
            {
                if (ulStatus == 0)
                {
                    Error_inf error_msg = {.id = err, .source = CAMERA, .response_type = 0};
                    xQueueSend(error_queue, &error_msg,0);
                    connection_establish = false;
                    break; 
                }
            }

            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb)
            {
                Error_inf error_msg = {.id = 2, .source = CAMERA, .response_type = 0};
                xQueueSend(error_queue, &error_msg, 0);
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
                Error_inf error_msg = {.id = 3, .source = CAMERA, .response_type = 1};
                xQueueSend(error_queue, &error_msg, 0);
                connection_establish = false;
            }

            vTaskDelay(pdMS_TO_TICKS(40)); // Control de tasa de frames
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