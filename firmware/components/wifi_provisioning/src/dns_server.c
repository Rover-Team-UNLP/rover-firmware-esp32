#include "dns_server.h"

static void dns_task(void *pvParameters);

static TaskHandle_t dns_handler = NULL;
static int dns_socket_fd = -1;

esp_err_t dns_server_start()
{
    if (xTaskCreate(dns_task, "dns_server", 3072, NULL, 5, &dns_handler) == pdPASS)
    {
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }
}

void dns_server_stop(void)
{

    if (dns_socket_fd != -1)
    {
        close(dns_socket_fd);
        dns_socket_fd = -1;
    }

    if (dns_handler != NULL)
    {
        vTaskDelete(dns_handler);
        dns_handler = NULL;
    }
}

static void dns_task(void *pvParameters)
{
    uint8_t trace[] = {0xC0, 0x0C, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x04, 192, 168, 4, 1};
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(53),
    };
    dns_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (dns_socket_fd < 0)
    {
        vTaskDelete(NULL);
        return;
    }
    int bind_ret = bind(dns_socket_fd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (bind_ret < 0)
    {
        close(dns_socket_fd);
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        uint8_t rx_buffer[128];
        struct sockaddr_in source_addr = {0};
        socklen_t socklen = sizeof(source_addr);

        int len = recvfrom(dns_socket_fd, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

        if (len < 0)
            break;

        if (len > 12 && (rx_buffer[2] & 0x80) == 0)
        {
            rx_buffer[2] = 0x81;
            rx_buffer[3] = 0x80;

            rx_buffer[6] = 0x00;
            rx_buffer[7] = 0x01;
            rx_buffer[8] = 0x00;
            rx_buffer[9] = 0x00;
            rx_buffer[10] = 0x00;
            rx_buffer[11] = 0x00;

            if (len + sizeof(trace) <= sizeof(rx_buffer))
            {

                memcpy(rx_buffer + len, trace, sizeof(trace));
                len += sizeof(trace);
                sendto(dns_socket_fd, rx_buffer, len, 0, (struct sockaddr *)&source_addr, socklen);
            }
        }
    }
    if (dns_socket_fd != -1)
    {
        close(dns_socket_fd);
        dns_socket_fd = -1;
    }
    vTaskDelete(NULL);
}