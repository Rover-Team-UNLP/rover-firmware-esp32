#include "nvs_storage.h"

static const char * STORAGE_NAMESPACE = "wifi_data";
nvs_handle_t   memory_handle;
static const char* ssid_key = "ssid";
static const char* passw_key = "passw";
size_t ssid_length = MAX_SSID_LENGTH; 
size_t passw_length = MAX_PASSW_LENGTH;



esp_err_t storage_init () {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    return err; 
}


esp_err_t storage_set_credentials (char * ssid, char * passwd) {
        esp_err_t  err = nvs_open(STORAGE_NAMESPACE,NVS_READWRITE, &memory_handle);
        if (err == ESP_OK) {
            err = nvs_set_str(memory_handle,ssid_key, ssid);
            if (err == ESP_OK) {
                err = nvs_set_str(memory_handle,passw_key, passwd);
                if (err == ESP_OK) {
                err = nvs_commit(memory_handle);
                }
            }
        }
        nvs_close(memory_handle);
        return err;
    }


esp_err_t storage_get_credentials (char * ssid, char * passwd) {
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &memory_handle);
    if (err == ESP_OK) {
        err = nvs_get_str(memory_handle,ssid_key,ssid, &ssid_length);
        if (err == ESP_OK) {
            err = nvs_get_str(memory_handle,passw_key,passwd, &passw_length);
        }
    }

    nvs_close(memory_handle);
    return err;
}