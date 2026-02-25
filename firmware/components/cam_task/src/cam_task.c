/* ======================================
 * File: cam_task.c
 * Description: MJPEG camera capture and HTTP chunked upload to /video/upload
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */
#include "cam_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "CAM_TASK";

#define CAM_SERVER_URL_MAX 64
static char s_cam_server_url[CAM_SERVER_URL_MAX] = "http://192.168.1.34:8080/video/upload";
const char *CAM_SERVER_URL = s_cam_server_url;

void cam_task_set_server_url(const char *url)
{
    if (url == NULL)
        return;
    strncpy(s_cam_server_url, url, sizeof(s_cam_server_url) - 1);
    s_cam_server_url[sizeof(s_cam_server_url) - 1] = '\0';
}

static TaskHandle_t cam_task_handle = NULL;
static camera_state_t last_camera_state = CAM_STATE_NORMAL;
static TaskHandle_t s_cam_upload_task_handle = NULL;
static volatile bool s_cam_suspended = false;

#define HTTP_TIMEOUT_MS 800
#define HTTP_ERROR_STREAK_BACKOFF_THRESHOLD 5
#define HTTP_ERROR_BACKOFF_BASE_MS 500
#define HTTP_ERROR_BACKOFF_MAX_MS 5000

static esp_http_client_handle_t s_http_client = NULL;
static uint32_t s_frame_count = 0;
static uint8_t s_http_error_streak = 0;
static TickType_t s_last_http_error_tick = 0;

static void handle_http_error_streak(void)
{
    if (s_http_error_streak < UINT8_MAX)
    {
        s_http_error_streak++;
    }
    s_last_http_error_tick = xTaskGetTickCount();
}

static void http_client_close_and_reset(void)
{
    if (s_http_client != NULL)
    {
        esp_http_client_close(s_http_client);
        esp_http_client_cleanup(s_http_client);
        s_http_client = NULL;
    }
    s_frame_count = 0;
}

#define CAM_LATEST_JPEG_BUF_SIZE (32 * 1024)
static uint8_t s_latest_jpeg_buf0[CAM_LATEST_JPEG_BUF_SIZE];
static uint8_t s_latest_jpeg_buf1[CAM_LATEST_JPEG_BUF_SIZE];
static volatile uint8_t s_latest_buf_idx = 0;
static volatile size_t s_latest_jpeg_len = 0;
static volatile uint32_t s_latest_seq = 0;
static portMUX_TYPE s_latest_mux = portMUX_INITIALIZER_UNLOCKED;

esp_err_t http_client_event_handler(esp_http_client_event_t *);

static void camera_upload_task(void *pvParameters);

static bool latest_frame_snapshot(const uint8_t **buf, size_t *len, uint32_t *seq)
{
    uint8_t idx;
    size_t l;
    uint32_t s;

    portENTER_CRITICAL(&s_latest_mux);
    idx = s_latest_buf_idx;
    l = s_latest_jpeg_len;
    s = s_latest_seq;
    portEXIT_CRITICAL(&s_latest_mux);

    if (s == 0 || l == 0)
    {
        return false;
    }

    *buf = (idx == 0) ? s_latest_jpeg_buf0 : s_latest_jpeg_buf1;
    *len = l;
    *seq = s;
    return true;
}

static void latest_frame_store(const uint8_t *src, size_t len)
{
    uint8_t next_idx = (s_latest_buf_idx == 0) ? 1 : 0;
    uint8_t *dst = (next_idx == 0) ? s_latest_jpeg_buf0 : s_latest_jpeg_buf1;

    memcpy(dst, src, len);

    portENTER_CRITICAL(&s_latest_mux);
    s_latest_buf_idx = next_idx;
    s_latest_jpeg_len = len;
    s_latest_seq++;
    portEXIT_CRITICAL(&s_latest_mux);
}

static int http_write_exact(esp_http_client_handle_t client, const char *data, int len)
{
    int total = 0;
    while (total < len)
    {
        int w = esp_http_client_write(client, data + total, len - total);
        if (w < 0)
        {
            return w;
        }
        total += w;
    }
    return total;
}

static int http_write_chunk(esp_http_client_handle_t client, const uint8_t *data, size_t len)
{
    char chunk_hdr[16];
    int hdr_len = snprintf(chunk_hdr, sizeof(chunk_hdr), "%x\r\n", (unsigned int)len);
    if (hdr_len <= 0 || hdr_len >= (int)sizeof(chunk_hdr))
    {
        return -1;
    }

    int w = http_write_exact(client, chunk_hdr, hdr_len);
    if (w < 0)
        return w;

    if (len > 0)
    {
        w = http_write_exact(client, (const char *)data, (int)len);
        if (w < 0)
            return w;
    }

    w = http_write_exact(client, "\r\n", 2);
    if (w < 0)
        return w;

    return (int)len;
}

void camera_adjust_quality(camera_state_t state)
{
    if (state == last_camera_state)
    {
        return;
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
        ESP_LOGI(TAG, "Camera set to NORMAL mode (QQVGA, q=%d)", CAM_NORMAL_QUALITY);
        break;

    case CAM_STATE_DEGRADED:
        sensor->set_framesize(sensor, CAM_DEGRADED_FRAMESIZE);
        sensor->set_quality(sensor, CAM_DEGRADED_QUALITY);
        ESP_LOGW(TAG, "Camera set to DEGRADED mode (QQVGA, q=%d)", CAM_DEGRADED_QUALITY);
        break;

    case CAM_STATE_SUSPENDED:
        ESP_LOGW(TAG, "Camera entering SUSPENDED mode");
        break;
    }

    last_camera_state = state;
}

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

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = CAM_NORMAL_FRAMESIZE,
        .jpeg_quality = CAM_JPEG_QUALITY_INIT,
        .fb_count = 4,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_LATEST};

    esp_err_t error = esp_camera_init(&config);

    return error;
}

void camera_task(void *pvParameters)
{
    cam_task_handle = xTaskGetCurrentTaskHandle();
    camera_state_t current_state;

    ESP_LOGI(TAG, "Camera task started (capture)");

    if (s_cam_upload_task_handle == NULL)
    {
        xTaskCreate(camera_upload_task, "camera_ul_task", 4096, NULL, 2, &s_cam_upload_task_handle);
    }

    while (1)
    {
        current_state = error_control_get_camera_state();

        if (current_state == CAM_STATE_SUSPENDED)
        {
            s_cam_suspended = true;
            ESP_LOGW(TAG, "Camera suspended - waiting for system to recover...");
            while (error_control_get_camera_state() == CAM_STATE_SUSPENDED)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            ESP_LOGI(TAG, "Camera resuming operation");
            current_state = error_control_get_camera_state();
        }
        s_cam_suspended = false;

        camera_adjust_quality(current_state);

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGW(TAG, "Failed to capture frame");
            Error_inf error_msg = {.id = 2, .source = CAMERA, .response_type = 0, .general_errors = FRAME_NULL};
            xQueueSend(to_error_queue, &error_msg, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (fb->len > CAM_LATEST_JPEG_BUF_SIZE)
        {
            esp_camera_fb_return(fb);
            continue;
        }

        size_t jpeg_len = fb->len;
        latest_frame_store(fb->buf, jpeg_len);
        esp_camera_fb_return(fb);

        if (s_cam_upload_task_handle != NULL)
        {
            xTaskNotifyGive(s_cam_upload_task_handle);
        }

        vTaskDelay(pdMS_TO_TICKS(get_frame_delay_ms(current_state)));
    }
}

static void camera_upload_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI(TAG, "Camera upload task started (persistent POST)");

    uint32_t last_sent_seq = 0;
    bool stream_open = false;

    while (1)
    {
        if (s_cam_suspended)
        {
            if (stream_open)
            {
                http_client_close_and_reset();
                stream_open = false;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (s_http_error_streak >= HTTP_ERROR_STREAK_BACKOFF_THRESHOLD)
        {
            uint8_t backoff_step = s_http_error_streak - HTTP_ERROR_STREAK_BACKOFF_THRESHOLD;
            if (backoff_step > 3)
            {
                backoff_step = 3;
            }
            uint32_t backoff_ms = HTTP_ERROR_BACKOFF_BASE_MS << backoff_step;
            if (backoff_ms > HTTP_ERROR_BACKOFF_MAX_MS)
            {
                backoff_ms = HTTP_ERROR_BACKOFF_MAX_MS;
            }
            ESP_LOGW(TAG, "HTTP error streak=%u, applying upload backoff of %ums", s_http_error_streak, (unsigned int)backoff_ms);
            vTaskDelay(pdMS_TO_TICKS(backoff_ms));
        }

        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        const uint8_t *jpeg_buf = NULL;
        size_t jpeg_len = 0;
        uint32_t seq = 0;
        if (!latest_frame_snapshot(&jpeg_buf, &jpeg_len, &seq))
        {
            continue;
        }
        if (seq == last_sent_seq)
        {
            continue;
        }

        if (!stream_open)
        {
            if (s_http_client == NULL)
            {
                esp_http_client_config_t config = {
                    .url = CAM_SERVER_URL,
                    .method = HTTP_METHOD_POST,
                    .timeout_ms = HTTP_TIMEOUT_MS,
                };
                s_http_client = esp_http_client_init(&config);
                if (s_http_client != NULL)
                {
                    esp_http_client_set_header(s_http_client, "Content-Type", "multipart/x-mixed-replace; boundary=frame");
                }
            }

            esp_err_t err = ESP_FAIL;
            if (s_http_client != NULL)
            {
                err = esp_http_client_open(s_http_client, -1);
            }
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to open HTTP stream: %s", esp_err_to_name(err));
                Error_inf error_msg = {.id = err, .source = CAMERA, .response_type = 0, .general_errors = HTTP_CLIENT_NO_OPEN};
                xQueueSend(to_error_queue, &error_msg, 0);
                handle_http_error_streak();
                if (err == ESP_ERR_HTTP_CONNECT)
                {
                    handle_http_error_streak();
                }
                http_client_close_and_reset();
                stream_open = false;
                continue;
            }

            static const uint8_t initial_boundary[] = "--frame\r\n";
            if (http_write_chunk(s_http_client, initial_boundary, sizeof(initial_boundary) - 1) < 0)
            {
                handle_http_error_streak();
                http_client_close_and_reset();
                stream_open = false;
                continue;
            }

            stream_open = true;
        }

        char part_header[128];
        int header_len = snprintf(part_header, sizeof(part_header),
                                  "Content-Type: image/jpeg\r\n"
                                  "Content-Length: %u\r\n\r\n",
                                  (unsigned int)jpeg_len);
        if (header_len <= 0 || header_len >= (int)sizeof(part_header))
        {
            continue;
        }

        static const uint8_t part_tail[] = "\r\n--frame\r\n";

        if (http_write_chunk(s_http_client, (const uint8_t *)part_header, (size_t)header_len) < 0 ||
            http_write_chunk(s_http_client, jpeg_buf, jpeg_len) < 0 ||
            http_write_chunk(s_http_client, part_tail, sizeof(part_tail) - 1) < 0)
        {
            Error_inf error_msg = {.id = 3, .source = CAMERA, .response_type = 0, .general_errors = WRITE_ERROR};
            xQueueSend(to_error_queue, &error_msg, 0);
            handle_http_error_streak();
            http_client_close_and_reset();
            stream_open = false;
            continue;
        }

        if (s_http_error_streak > 0)
        {
            ESP_LOGI(TAG, "HTTP stream recovered, resetting error streak (was %u)", s_http_error_streak);
        }
        s_http_error_streak = 0;
        last_sent_seq = seq;
        s_frame_count++;
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