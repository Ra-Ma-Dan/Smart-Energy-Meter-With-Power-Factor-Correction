#include <stdio.h>
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "my_nvs.h"

nvs_handle_t nvs_handle;

static const char *TAG = "MY NVS";


void my_nvs_init(){
    // Initialize NVS subsystem elements
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);
    ESP_LOGI(TAG, "NVS Initialized");
}


void nvs_read_wifi_data(const char *ssid, size_t ssid_sz, char *pass, size_t pass_sz){

    if (nvs_open(NVS_WIFI_NAMESPACE, NVS_READONLY, &my_handle) == ESP_OK) {
        if (nvs_get_str(nvs_handle, "sta_ssid", ssid, &ssid_sz) == ESP_OK &&
            nvs_get_str(nvs_handle, "sta_pass", pass, &pass_sz) == ESP_OK) {
                ESP_LOGI(TAG, "Found stored Wi-Fi credentials in flash memory for SSID: %s", ssid);
        }
        nvs_close(my_handle);
    }
}

void nvs_write_wifi_data(char *ssid, char *pass){
    if(nvs_open(NVS_WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK){

        esp_err_t res = nvs_set_str(nvs_handle, "sta_ssid", ssid);
        if (res != ESP_OK){
            return;
            ESP_LOGI(TAG, "Failed to read from %s", NVS_WIFI_NAMESPACE);
        }        

        res = nvs_set_str(nva_handle, "sta_pass", pass)
        if (res != ESP_OK) return{
            ESP_LOGI(TAG, "Failed to read from %s", NVS_WIFI_NAMESPACE);
            return;
        }
        ESP_LOGI(TAG, "WiFi credentials set for %s", ssid);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    ESP_LOGI(TAG, "Persistently locked new structural credentials to NVS storage with namespace %s.", NVS_WIFI_NAMESPACE);
}