#include "cam_task.h"

esp_err_t uri_get_handler(httpd_req_t *req);


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

httpd_handle_t server_init() {
    httpd_config_t server_config =  HTTPD_DEFAULT_CONFIG();
    server_config.task_priority = 1; //For the first try we use the server on low priority. 
    httpd_handle_t server = NULL;

    httpd_uri_t uri_get = {
        .uri  = "/stream",
        .method = HTTP_GET,
        .handler = uri_get_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &server_config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
    };

    return server;
}


esp_err_t uri_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    
    while(1) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            continue;
        }
        
        char header[64];
        int header_len = snprintf(header, sizeof(header),
            "--frame\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: %u\r\n\r\n", fb->len);
        
        httpd_resp_send_chunk(req, header, header_len);

        httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);

        httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb);

        vTaskDelay(pdMS_TO_TICKS(50)); //20 FPS
    }
    return ESP_OK; 
}