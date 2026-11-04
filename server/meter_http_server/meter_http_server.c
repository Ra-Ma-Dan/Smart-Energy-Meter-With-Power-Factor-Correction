#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "meter_http_server.h"
#include "esp_http_server.h"


static const char *TAG = "HTTP_SERVER";

// 1. Embed the binary text files out of flash memory strings
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern const uint8_t style_css_end[]    asm("_binary_style_css_end");
extern const uint8_t scripts_js_start[] asm("_binary_scripts_js_start");
extern const uint8_t scripts_js_end[]   asm("_binary_scripts_js_end");


// Handler for HTML Webpage (root /)
static esp_err_t root_get_handler(httpd_req_t *request) {
    size_t len = index_html_end - index_html_start;
    httpd_resp_set_type(request, "text/html");
    httpd_resp_send(request, (const char *)index_html_start, len);
    return ESP_OK;
}

// Handler for CSS Stylesheet
static esp_err_t css_get_handler(httpd_req_t *request){
    size_t len = style_css_end - style_css_start;
    httpd_resp_set_type(request, "text/css");
    httpd_resp_send(request, (const char *)style_css_start, len);
    return ESP_OK;
}

// Handler for Javascript engine
static esp_err_t js_get_handler(httpd_req_t *request) {
    size_t len = scripts_js_end - scripts_js_start;
    httpd_resp_set_type(request, "application/javascript");
    httpd_resp_send(request, (const char *)scripts_js_start, len);
    return ESP_OK;
}


// 1. Relay Power ON/OFF Action Route Handler
static esp_err_t power_post_handler(httpd_req_t *request) {
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
static esp_err_t recharge_post_handler(httpd_req_t *request) {
    char buf[64];
    char val[16] = {0};
    
    // Extract URI query params (e.g., /api/recharge?amount=500)
    if (httpd_req_get_url_query_str(request, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "amount", val, sizeof(val)) == ESP_OK) {
            float topup_amount = atof(val);
            if (topup_amount > 0) {
                credit_balance += topup_amount;
                ESP_LOGI(TAG, "Balance credited successfully. New total: ₦%.2f", credit_balance);
                
                httpd_resp_set_status(request, "200 OK");
                httpd_resp_send(request, "Recharge successful", HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            }
        }
    }
    
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid recharge data payload");
    return ESP_FAIL;
}


// 2. The AJAX /data API handler returning structured JSON
static esp_err_t data_get_handler(httpd_req_t *request) {
    char json_string[256];
    
    // Convert current float parameters into an array format for JavaScript parsing
    snprintf(json_string, sizeof(json_string),
             "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy\":%.3f,\"frequency\":%.1f,\"pf\":%.2f,\"credit\":%.2f}",
             live_pfc_data.voltage, live_pfc_data.current, 
             live_pfc_data.power, (live_pfc_data.energy / 1000.0f), // Convert Wh to kWh
             live_pfc_data.frequency, live_pfc_data.power_factor,
             credit_balance, active_capacitor_mask);

    // snprintf(json_string, sizeof(json_string),
    //         "{\"voltage\":%.1f,\"current\":%.3f,\"power\":%.1f,\"energy\":%.3f,\"frequency\":%.1f,\"pf\":%.2f,\"credit\":%.2f}",
    //         live_pfc_data.voltage, live_pfc_data.current, 
    //         live_pfc_data.power, (live_pfc_data.energy / 1000.0f), 
    //         live_pfc_data.frequency, live_pfc_data.power_factor,
    //         credit_balance);


    httpd_resp_set_type(request, "application/json");
    // Enable CORS to make cross-origin debugging via Wokwi smooth
    httpd_resp_set_hdr(request, "Access-Control-Allow-Origin", "*"); 
    httpd_resp_send(request, json_string, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 3. New POST handler to clear the PZEM accumulated chip registers
static esp_err_t reset_post_handler(httpd_req_t *request) {
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


static const httpd_uri_t reset_uri = { 
    .uri = "/api/reset", 
    .method = HTTP_POST, 
    .handler = reset_post_handler 
};

static const httpd_uri_t power_control_uri = { 
    .uri = "/api/recharge?amount=${amount}", 
    .method = HTTP_POST, 
    .handler = recharge_post_handler 
};

static const httpd_uri_t credit_recgarge_uri = { 
    .uri = "/api/power?state=${state}", 
    .method = HTTP_POST, 
    .handler = recharge_post_handler 
};

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_HTTPD_WEB_PORT; // Pulls from your Kconfig file!

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &css_uri);
        httpd_register_uri_handler(server, &js_uri);
        httpd_register_uri_handler(server, &data_uri);
        httpd_register_uri_handler(server, &reset_uri);
        httpd_register_uri_handler(server, &credit_recgarge_uri);
        httpd_register_uri_handler(server, &power_control_uri);
    }
}

// Action: fetch('/api/power?state=1');
// Action: fetch('/api/power?state=0');
// Action: fetch(`/api/recharge?amount=${amount}`, { method: 'POST' });
