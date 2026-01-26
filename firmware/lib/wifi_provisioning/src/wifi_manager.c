#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_storage.h"
#include "http_portal.h"
#include "dns_server.h"

#define MAX_RETRY 5

static int s_retry_counter = 0;

EventGroupHandle_t wifi_event_group;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void start_sta(char *ssid, char *password);
static void start_ap_provisioning(void);

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (s_retry_counter < MAX_RETRY)
            {
                esp_wifi_connect();
                s_retry_counter++;
            }
            else
            {
                xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                s_retry_counter = 0;
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            s_retry_counter = 0;
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
}

static void start_sta(char *ssid, char *password)
{

    wifi_config_t wifi_config;

    esp_netif_create_default_wifi_sta();

    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

static void start_ap_provisioning()
{

    esp_netif_create_default_wifi_ap();
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Rover_Setup",
            .ssid_len = strlen("Rover_Setup"),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
            .ssid_hidden = 0},
    };

    if (strlen((char *)wifi_config.ap.password) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    dns_server_start();
    http_portal_start();
}

void wifi_provisioning_init(void)
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

    char ssid[32] = {0};
    char password[64] = {0};

    esp_err_t err = storage_get_credentials(ssid, password);

    if (err == ESP_OK)
    {
        start_sta(ssid, password);

        EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        if (bits & WIFI_FAIL_BIT)
        {
            esp_wifi_stop();
            esp_wifi_set_mode(WIFI_MODE_NULL);
            start_ap_provisioning();
        }
    }
    else
    {
        start_ap_provisioning();
    }
}