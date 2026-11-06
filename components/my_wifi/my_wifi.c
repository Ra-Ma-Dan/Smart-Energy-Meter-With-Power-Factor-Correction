#include <stdio.h>
#include <string.h>
#include "my_wifi.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"

#include "meter_http_server.h"

static const char *STA_TAG = "WiFi_STA";
static const char *AP_TAG = "SoftAP";

// Allocating space for globals declared as extern in header
int num_dev_connected = 0;
EventGroupHandle_t wifi_enent_handle;
int num_tries = 0;

// FIXED: event_id argument updated to standard int32_t to match esp_event system requirements
static void wifi_event_handler_fxn(void *args, esp_event_base_t event_base, int32_t event_id, void *event_info)
{
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED){
        wifi_event_ap_staconnected_t * event = (wifi_event_ap_staconnected_t *)event_info;
        ESP_LOGI(AP_TAG, "Station "MACSTR" Joined, AID=%d", MAC2STR(event->mac), event->aid);
        num_dev_connected++;
        
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED){
        wifi_event_ap_stadisconnected_t * event = (wifi_event_ap_stadisconnected_t *)event_info;
        ESP_LOGI(AP_TAG, "Station "MACSTR" Left, AID=%d: Reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        num_dev_connected--;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if(num_tries < MAX_RETRY){
            ESP_LOGI(STA_TAG, "Disconnected. Retrying... (%d/%d)", num_tries, MAX_RETRY);
            
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_wifi_connect();
            num_tries++;
        } else {
            ESP_LOGI(STA_TAG, "Failed to reconnect");
            // FIXED: corrected variable name spelling from wifi_event_handle
            xEventGroupSetBits(wifi_enent_handle, WIFI_DISCONNECTED_BIT);
            num_tries = 0;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_info;
        // FIXED: Removed invalid event->aid reference which does not exist in IP_EVENT
        ESP_LOGI(STA_TAG, "Connected. IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_enent_handle, WIFI_CONNECTED_BIT);

        start_webserver();
        ESP_LOGI(STA_TAG, "Web Server started at port: %d", CONFIG_HTTPD_WEB_PORT);
    }
}

void wifi_sta_init(void)
{
    wifi_enent_handle = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // FIXED: Changed from creating an AP interface to a station client interface
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // FIXED: Replaced legacy ESP_WIFI_ANY_ID with standard ESP_EVENT_ANY_ID for modern ESP-IDF frameworks
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_fxn, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_fxn, NULL, NULL);

    wifi_config_t wifi_sta_cfg = {
        .sta = {
            .ssid = CONFIG_TARGET_WIFI_SSID,
            .password = CONFIG_TARGET_WIFI_PASS,
            .channel  = STA_CHANNEL,
            .scan_method = STA_SCAN_METHOD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_enent_handle,
                                            WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            pdMS_TO_TICKS(10000));
    
    if(bits & WIFI_CONNECTED_BIT){
        ESP_LOGI(STA_TAG, "Station connected successfully");
    } else if(bits & WIFI_DISCONNECTED_BIT){
        ESP_LOGI(STA_TAG, "Station Disconnected completely");
    } else {
        ESP_LOGI(STA_TAG, "Unexpected Event connection timeline");
    }

    ESP_LOGI(STA_TAG, "Finished WIFI_STA Initialization");
}

void my_wifi_init(void)
{
    esp_err_t res = nvs_flash_init();
    if(res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND){
        nvs_flash_erase();
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);

    wifi_sta_init();
}
