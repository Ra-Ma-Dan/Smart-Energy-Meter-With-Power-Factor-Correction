#include <stdio.h>
#include "my_wifi.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "event_groups.h"

static const char *TAG = "WiFi";

// EventGroupHandle_t wifi_enent_handle;

void IRAM_ATTR wifi_event_handler_fxn(void *args, esp_event_base_t event_base, int16_t event_id, void *event_info)
{
    if(event_id == WIFI_EVENT_AP_STACONNECTED){
        wifi_event_ap_staconnected_t * event = (wifi_event_ap_staconnected_t *)event_info;
        ESP_LOGI(TAG, "Station "MACSTR" Joinned, AID=%d", MAC2STR(event->mac), event->aid);
        // xEventGroupSetBits(wifi_event_handle, AP_STACONNECTED_BIT);
        num_dev_connected++;

    } else if(event_id == WIFI_EVENT_AP_STADISCONNECTED){
        wifi_event_ap_stadisconnected_t * event = (wifi_event_ap_stadisconnected_t *)event_info;
        ESP_LOGI(TAG, "Station "MACSTR" Left, AID=%d: Reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        // xEventGroupSetBits(wifi_event_handle, AP_STADISCONNECTED_BIT);
        num_dev_connected--;
    }
}

void wifi_ap_init()
{
    // wifi_enent_handle = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_event_hanlder_instance_register(WIFI_EVENT, ESP_ANY_EVENT, &wifi_event_handler_fxn, NULL, NULL);

    wifi_config_t wifi_ap_cfg = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_NETWORK_NAME,
            .ssid_len = strlen(CONFIG_ESP_WIFI_NETWORK_NAME),
            .password = CONFIG_ESP_WIFI_NETWORK_PASS,
            .channel  = CONFIG_ESP_WIFI_NETWORK_CHANNEL,
            .max_connection = CONFIG_MAX_AP_CONNECTION,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(ESP_WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_WIFI_IF_AP, &wifi_ap_cfg));

    EventBits_t bits = xEventGroupWaitBits(wifi_enent_handle,
                                            AP_STACONNECTED_BIT | AP_STADISCONNECTED_BIT,
                                            pdFALSE,
                                            pdFALSE
                                            pdMS_TO_TICKS(10000));
    
    // if(bits & AP_STACONNECTED_BIT){
    //     ESP_LOGI(TAG, "Staion connected");
    // } else if(bits & AP_STADISCONNECTED_BIT){
    //     ESP_LOGI(TAG, "Staion Disconnected");
    // } else {
    //     ESP_LOGI(TAG, "Unexpected Event");
    // }

    ESP_ERROR_CHECK(esp_wifi_start());

}

void my_wifi_init(void)
{
    esp_err_t res = nvs_flash_init();
    if(res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND){
        nvs_flash_erase();
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);

    wifi_ap_init();
}
