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
#include "my_nvs.h"

static const char *STA_TAG = "WiFi_STA";
static const char *AP_TAG = "SoftAP";

int num_dev_connected = 0;
EventGroupHandle_t wifi_enent_handle;
int num_tries = 0;

static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;

static void wifi_event_handler_fxn(void *args, esp_event_base_t event_base, int32_t event_id, void *event_info)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_info;
        ESP_LOGI(AP_TAG, "Local Client "MACSTR" Joined SoftAP dashboard, AID=%d", MAC2STR(event->mac), event->aid);
        num_dev_connected++;
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_info;
        ESP_LOGI(AP_TAG, "Local Client "MACSTR" Left SoftAP dashboard, AID=%d", MAC2STR(event->mac), event->aid);
        num_dev_connected--;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Only initiate manual system connect requests if credentials exist
        wifi_config_t current_cfg;
        esp_wifi_get_config(WIFI_IF_STA, &current_cfg);
        if (strlen((char*)current_cfg.sta.ssid) > 0) {
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (num_tries < MAX_RETRY) {
            ESP_LOGI(STA_TAG, "Station interface lost uplink link. Retrying... (%d/%d)", num_tries + 1, MAX_RETRY);
            esp_wifi_connect();
            num_tries++;
        } else {
            ESP_LOGW(STA_TAG, "Station failed to reach local router. Staying offline on cloud (SoftAP is still running).");
            xEventGroupSetBits(wifi_enent_handle, WIFI_DISCONNECTED_BIT);
            num_tries = 0;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_info;
        ESP_LOGI(STA_TAG, "Connected to Router Internet! STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_enent_handle, WIFI_CONNECTED_BIT);
        // ThingsBoard initialization tasks can safely launch here
    }
}

// 📡 NEW: Runtime background function to apply credentials and connect
void wifi_connect_to_new_station(const char *ssid, const char *password)
{
    wifi_config_t wifi_sta_cfg = {0};
    
    strncpy((char*)wifi_sta_cfg.sta.ssid, ssid, sizeof(wifi_sta_cfg.sta.ssid));
    strncpy((char*)wifi_sta_cfg.sta.password, password, sizeof(wifi_sta_cfg.sta.password));
    wifi_sta_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_LOGI(STA_TAG, "Applying updated station profile structure configurations...");
    
    num_tries = 0;
    xEventGroupClearBits(wifi_enent_handle, WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT);

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_connect());

}

void my_wifi_init(void)
{   
    my_nvs_init();
    wifi_enent_handle = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Allocate BOTH adapters into operational structures
    sta_netif = esp_netif_create_default_wifi_sta();
    ap_netif  = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_fxn, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_fxn, NULL, NULL));

    // Define Local Offline Fallback SoftAP Parameters
    wifi_config_t wifi_ap_cfg = {
        .ap = {
            .ssid = "SmartMeter_Dash",
            .ssid_len = strlen("SmartMeter_Dash"),
            .password = "87654321",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = 1
        },
    };

    // Attempt to pull runtime connection credentials from persistent NVS profile storage
    wifi_config_t wifi_sta_cfg = {0};
    char stored_ssid[32] = {0};
    char stored_pass[64] = {0};
    size_t ssid_sz = sizeof(stored_ssid);
    size_t pass_sz = sizeof(stored_pass);


    void nvs_read_data(stored_ssid, ssid_sz, stored_pass, pass_sz);
    if(ssid_sz > 0 && pass_sz > 0){
        strncpy((char*)wifi_sta_cfg.sta.ssid, stored_ssid, sizeof(wifi_sta_cfg.sta.ssid));
        strncpy((char*)wifi_sta_cfg.sta.password, stored_pass, sizeof(wifi_sta_cfg.sta.password));
        ESP_LOGI(STA_TAG, "Found stored Wi-Fi credentials in flash memory for SSID: %s", stored_ssid);
    }else{
        wifi_sta_cfg.sta.ssid = CONFIG_TARGET_WIFI_SSID;
        wifi_sta_cfg.sta.password = CONFIG_TARGET_WIFI_PASS;
    }
  
      
    // Set combined mode and apply settings
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_cfg));
    
    if (strlen((char*)wifi_sta_cfg.sta.ssid) > 0) {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_cfg));
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Always start the web server so the local interface handles incoming traffic instantly
    start_webserver();
    ESP_LOGI(AP_TAG, "Local Dashboard SoftAP running. Gateway online at IP: 192.168.4.1");
}
