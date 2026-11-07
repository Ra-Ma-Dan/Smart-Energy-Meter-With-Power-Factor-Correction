#include <stdio.h>
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "my_nvs.h"

nvs_handle_t wifi_nvs_handle;

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


esp_err_t nvs_read_wifi_data(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz){
    esp_err_t ret = nvs_open(NVS_WIFI_NAMESPACE, NVS_READONLY, &wifi_nvs_handle);
    if (ret == ESP_OK) {
        ret = nvs_get_str(wifi_nvs_handle, "sta_ssid", ssid, &ssid_sz);
        if(ret != ESP_OK )return ret;
        ret = nvs_get_str(wifi_nvs_handle, "sta_pass", pass, &pass_sz);
        if (ret != ESP_OK) return ret;
        
        ESP_LOGI(TAG, "Found stored Wi-Fi credentials in flash memory for SSID: %s", ssid);
        nvs_close(wifi_nvs_handle);
    } else {
        return ret;
    }
    return ESP_OK;
}

esp_err_t nvs_write_wifi_data(char *ssid, char *pass){
    if(nvs_open(NVS_WIFI_NAMESPACE, NVS_READWRITE, &wifi_nvs_handle) == ESP_OK){

        esp_err_t res = nvs_set_str(wifi_nvs_handle, "sta_ssid", ssid);
        if (res != ESP_OK){
            ESP_LOGI(TAG, "Failed to read ssid from %s", NVS_WIFI_NAMESPACE);
            return res;
        }        

        res = nvs_set_str(wifi_nvs_handle, "sta_pass", pass);
        if (res != ESP_OK){
            ESP_LOGI(TAG, "Failed to read pass from %s", NVS_WIFI_NAMESPACE);
            return res;
        }
        nvs_commit(wifi_nvs_handle);

        ESP_LOGI(TAG, "WiFi credentials set for %s", ssid);
        nvs_close(wifi_nvs_handle);
    }
    ESP_LOGI(TAG, "Persistently locked new structural credentials to NVS storage with namespace %s.", NVS_WIFI_NAMESPACE);
    return ESP_OK;
}