#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"


#include "meter_http_server.h"
#include "my_gpio.h"
#include "my_nvs.h"
#include "my_wifi.h"

static const char *TAG = "HTTP_SERVER";


bool power_state = true;
float credit_balance = 50.0f;

// 1. Embed the binary text files out of flash memory strings
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern const uint8_t style_css_end[]    asm("_binary_style_css_end");
extern const uint8_t scripts_js_start[] asm("_binary_scripts_js_start");
extern const uint8_t scripts_js_end[]   asm("_binary_scripts_js_end");


// Handler for HTML Webpage (root /)
static esp_err_t root_get_handler(httpd_req_t *request) 
{
    size_t len = index_html_end - index_html_start;
    httpd_resp_set_type(request, "text/html");
    httpd_resp_send(request, (const char *)index_html_start, len);
    return ESP_OK;
}

// Handler for CSS Stylesheet
static esp_err_t css_get_handler(httpd_req_t *request)
{
    size_t len = style_css_end - style_css_start;
    httpd_resp_set_type(request, "text/css");
    httpd_resp_send(request, (const char *)style_css_start, len);
    return ESP_OK;
}

// Handler for Javascript engine
static esp_err_t js_get_handler(httpd_req_t *request) 
{
    size_t len = scripts_js_end - scripts_js_start;
    httpd_resp_set_type(request, "application/javascript");
    httpd_resp_send(request, (const char *)scripts_js_start, len);
    return ESP_OK;
}


// 1. Relay Power ON/OFF Action Route Handler
static esp_err_t power_post_handler(httpd_req_t *request)
 {
    char buf[32];
    char val[10] = {0};
    
    // Extract URI query params (e.g., /api/power?state=1)
    if (httpd_req_get_url_query_str(request, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "state", val, sizeof(val)) == ESP_OK) {
            int state_val = atoi(val);
            power_state = (state_val == 1) ? true : false;
            ESP_LOGI(TAG, "Relay Target State Updated: %s", power_state ? "ON" : "OFF");
            
            httpd_resp_set_status(request, "200 OK");
            httpd_resp_send(request, "Power State Updated Successfully", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
    }
    
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing state key param");
    return ESP_FAIL;
}

// 2. Recharge Credit Top-Up Route Handler
static esp_err_t recharge_post_handler(httpd_req_t *request)
{
    char buf[64];
    char val[16] = {0};
    
    // Extract URI query params (e.g., /api/recharge?amount=500)
    if (httpd_req_get_url_query_str(request, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "amount", val, sizeof(val)) == ESP_OK) {
            float topup_amount = atof(val);
            if (topup_amount > 0) {
                
                // Check if our communication channel is initialized
                if (recharge_queue != NULL) {
                    // Send top-up amount to the queue. Wait up to 10ms if the queue is full.
                    if (xQueueSend(recharge_queue, &topup_amount, pdMS_TO_TICKS(10)) == pdPASS) {
                        ESP_LOGI(TAG, "Recharge of ₦%.2f pushed to task queue.", topup_amount);
                        
                        httpd_resp_set_status(request, "200 OK");
                        httpd_resp_send(request, "Recharge queued successfully", HTTPD_RESP_USE_STRLEN);
                        return ESP_OK;
                    } else {
                        ESP_LOGE(TAG, "Recharge queue full! Rejecting request.");
                        httpd_resp_set_status(request, "503 Service Unavailable");
                        return ESP_FAIL;
                    }
                }
            }
        }
    }
    
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid recharge data payload");
    return ESP_FAIL;
}



// 2. The AJAX /data API handler returning structured JSON
static esp_err_t data_get_handler(httpd_req_t *request)
 {
    char json_string[256];
    // Convert current float parameters into an array format for JavaScript parsing
    snprintf(json_string, sizeof(json_string),
             "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy\":%.3f,\"frequency\":%.1f,\"pf\":%.2f,\"credit\":%.2f,\"cap_banks\":%d}",
             live_pfc_data.voltage, live_pfc_data.current, 
             live_pfc_data.power, (live_pfc_data.energy / 1000.0f), // Convert Wh to kWh
             live_pfc_data.frequency, live_pfc_data.power_factor,
             credit_balance, active_capacitor_mask);


    httpd_resp_set_type(request, "application/json");
    // Enable CORS to make cross-origin debugging via Wokwi smooth
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*"); 
    httpd_resp_send(request, json_string, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 3. New POST handler to clear the PZEM accumulated chip registers
static esp_err_t reset_post_handler(httpd_req_t *request)
 {
    ESP_LOGI(TAG, "Web request received: Resetting energy metrics...");
    
    if (pzem_reset_energy() == ESP_OK) {
        httpd_resp_set_status(request, "200 OK");
        httpd_resp_send(request, "Reset Successful", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    } else {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Hardware Reset Failed");
        return ESP_FAIL;
    }
}

// 📡 NEW: Runtime Station Configuration URI Post Handler
static esp_err_t wifi_config_post_handler(httpd_req_t *request)
{
    char content[128];
    int ret, received = 0;
    int total_len = request->content_len;

    if (total_len >= sizeof(content)) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Payload too large");
        return ESP_FAIL;
    }

    // Read inbound raw JSON configuration payload
    while (received < total_len) {
        ret = httpd_req_recv(request, content + received, total_len - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return ESP_FAIL;
        }
        received += ret;
    }
    content[received] = '\0';

    char target_ssid[32] = {0};
    char target_pass[64] = {0};

    // Compact JSON key lookup parsing expressions
    char *ssid_ptr = strstr(content, "\"ssid\":\"");
    char *pass_ptr = strstr(content, "\"password\":\"");

    if (ssid_ptr) {
        ssid_ptr += 8; // Offset past key label length
        char *end = strchr(ssid_ptr, '"');
        if (end) strncpy(target_ssid, ssid_ptr, end - ssid_ptr);
    }
    if (pass_ptr) {
        pass_ptr += 12; // Offset past key label length
        char *end = strchr(pass_ptr, '"');
        if (end) strncpy(target_pass, pass_ptr, end - pass_ptr);
    }

    if (strlen(target_ssid) == 0) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Missing target SSID token");
        return ESP_FAIL;
    }

    
    nvs_write_wifi_data(target_ssid, target_pass); //Save Wifi credentials to NVS
    ESP_LOGI(TAG, "New Wi-Fi Profile Inbound! SSID: %s and saved to NVS", target_ssid);

    // Send response statement back to UI context before shifting physical adapters
    httpd_resp_set_status(request, "200 OK");
    httpd_resp_send(request, "Credentials Saved. Connecting...", HTTPD_RESP_USE_STRLEN);

    // Call dynamic background task reconnect sequence
    wifi_connect_to_new_station(target_ssid, target_pass);

    return ESP_OK;
}


// Register URI Paths
static const httpd_uri_t root_uri = { 
    .uri = "/", 
    .method = HTTP_GET, 
    .handler = root_get_handler 
};
static const httpd_uri_t css_uri  = { 
    .uri = "/style.css", 
    .method = HTTP_GET, 
    .handler = css_get_handler 
};
static const httpd_uri_t js_uri   = { 
    .uri = "/scripts.js", 
    .method = HTTP_GET, 
    .handler = js_get_handler 
};
static const httpd_uri_t data_uri = { 
    .uri = "/data", 
    .method = HTTP_GET, 
    .handler = data_get_handler 
};
static const httpd_uri_t reset_uri = { 
    .uri = "/api/reset", 
    .method = HTTP_POST, 
    .handler = reset_post_handler 
};
static const httpd_uri_t credit_recgarge_uri = { 
    .uri = "/api/recharge", 
    .method = HTTP_POST, 
    .handler = recharge_post_handler
};

static const httpd_uri_t power_control_uri = { 
    .uri = "/api/power", 
    .method = HTTP_POST, 
    .handler = power_post_handler 
};

// New Configuration registration token 
static const httpd_uri_t wifi_config_uri = { 
    .uri = "/api/wifi/config", 
    .method = HTTP_POST, 
    .handler = wifi_config_post_handler 
};

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_HTTPD_WEB_PORT; // Pulls from your Kconfig file!
    
    if (server != NULL) {
        return; // Prevent duplicate instantiation instances
    }

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &css_uri);
        httpd_register_uri_handler(server, &js_uri);
        httpd_register_uri_handler(server, &data_uri);
        httpd_register_uri_handler(server, &reset_uri);
        httpd_register_uri_handler(server, &credit_recgarge_uri);
        httpd_register_uri_handler(server, &power_control_uri);
        httpd_register_uri_handler(server, &wifi_config_uri); // Added new provisioning endpoint
    }
}

