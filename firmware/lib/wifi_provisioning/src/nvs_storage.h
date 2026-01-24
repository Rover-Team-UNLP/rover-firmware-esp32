#ifndef NVS_STORAGE
#define NVS_STORAGE

#include <nvs_flash.h>
#include <nvs.h>
#include <stdint.h>

#define MAX_SSID_LENGTH 32
#define MAX_PASSW_LENGTH 64

/**
 * @brief Initialize the Non-Volatile Storage (NVS) flash partition.
 *
 * This function handles the low-level initialization of the default NVS partition.
 * If the partition is truncated or contains a newer version than the software
 * can understand, it will erase the partition and initialize it again.
 *
 * @return
 * - ESP_OK: Storage initialized successfully.
 * - ESP_ERR_NVS_NO_FREE_PAGES: The NVS partition was full (requires erase).
 * - ESP_ERR_NVS_NEW_VERSION_FOUND: Newer format version found (requires erase).
 * - Others: Failure codes from the underlying nvs_flash_init.
 */
esp_err_t storage_init();

/**
 * @brief Retrieve WiFi credentials from NVS.
 *
 * Opens the specific namespace for networking and attempts to read the SSID
 * and password strings. This is used during the boot process to decide
 * whether to start in Station (STA) mode or Access Point (AP) mode.
 *
 * @param[out] ssid      Pointer to the buffer where the SSID will be stored.
 * @param[out] password  Pointer to the buffer where the password will be stored.
 *
 * @return
 * - ESP_OK: Credentials retrieved successfully.
 * - ESP_ERR_NVS_NOT_FOUND: No credentials have been saved yet.
 * - ESP_ERR_NVS_INVALID_LENGTH: The provided buffer is too small for the stored string.
 */
esp_err_t storage_get_credentials(char * ssid, char * password);

/**
 * @brief Persist WiFi credentials to NVS.
 *
 * Writes the provided SSID and password to the NVS flash. This function
 * ensures that the data is physically written to the flash by calling a
 * commit command after the write operation.
 *
 * @param[in] ssid      The SSID string received from the provisioning portal.
 * @param[in] password  The password string received from the provisioning portal.
 *
 * @return
 * - ESP_OK: Data saved and committed successfully.
 * - ESP_FAIL: Error during the write or commit process.
 */
esp_err_t storage_set_credentials(char * ssid, char * password);



#endif