#include "nvs_storage.h"


esp_err_t storage_init () {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    return err; 
}


esp_err_t storage_set_credentials (char * ssid, char * passwd) {

}