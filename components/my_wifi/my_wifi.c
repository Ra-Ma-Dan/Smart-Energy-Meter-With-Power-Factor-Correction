#include <stdio.h>
#include "my_wifi.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "freertos/event_groups.h"

static const char *STA_TAG = "WiFi_STA";
static const char *AP_TAG = "SoftAP";


int num_tries = 0;

static void wifi_event_handler_fxn(void *args, esp_event_base_t event_base, int16_t event_id, void *event_info)
{
    if(event_id == WIFI_EVENT_AP_STACONNECTED){
        wifi_event_ap_staconnected_t * event = (wifi_event_ap_staconnected_t *)event_info;
        ESP_LOGI(AP_TAG, "Station "MACSTR" Joinned, AID=%d", MAC2STR(event->mac), event->aid);
        num_dev_connected++;
        
    } else if(event_id == WIFI_EVENT_AP_STADISCONNECTED){
        wifi_event_ap_stadisconnected_t * event = (wifi_event_ap_stadisconnected_t *)event_info;
        ESP_LOGI(AP_TAG, "Station "MACSTR" Left, AID=%d: Reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        num_dev_connected--;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if(num_tries < MAX_RETRY){
            esp_wifi_connect();
            ESP_LOGI(STA_TAG, "Disconnected. Retrying...");
            num_tries++;
        } else {
            ESP_LOGI(STA_TAG, "Failed to reconnect");
            xEventGroupSetBits(wifi_event_handle, WIFI_DISCONNECTED_BIT);
            num_tries = 0;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_info;
        ESP_LOGI(STA_TAG, "Connected to SSID: %s, IP: " IPSTR " and AID: %d", IP2STR(event->ip_info.ip), event->aid);
        xEventGroupSetBits(wifi_event_handle, WIFI_CONNECTED_BIT);
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

    esp_event_handler_instance_register(WIFI_EVENT, ESP_WIFI_ANY_ID, &wifi_event_handler_fxn, NULL, NULL);

    wifi_config_t wifi_ap_cfg = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_NETWORK_NAME,
            .ssid_len = strlen(CONFIG_ESP_WIFI_NETWORK_NAME),
            .password = CONFIG_ESP_WIFI_NETWORK_PASS,
            .channel  = CONFIG_ESP_WIFI_NETWORK_CHANNEL,
            .max_connection = CONFIG_MAX_AP_CONNECTION,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_cfg));


    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(AP_TAG, "Finished SoftAP Initialization");
}


void wifi_sta_init()
{
    wifi_enent_handle = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_WIFI_ANY_ID, &wifi_event_handler_fxn, NULL, NULL);

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

    EventBits_t bits = xEventGroupWaitBits(wifi_enent_handle,
                                            WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT,
                                            pdFALSE,
                                            pdFALSE
                                            pdMS_TO_TICKS(10000));
    
    if(bits & AP_STACONNECTED_BIT){
        ESP_LOGI(STA_TAG, "Staion connected");
    } else if(bits & AP_STADISCONNECTED_BIT){
        ESP_LOGI(STA_TAG, "Staion Disconnected");
    } else {
        ESP_LOGI(STA_TAG, "Unexpected Event");
    }

    ESP_ERROR_CHECK(esp_wifi_start());
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

    wifi_ap_init();
    wifi_sta_init();
}
