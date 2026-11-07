#ifndef MY_NVS_H
#define MY_NVS_H

#include "esp_err.h"

#define NVS_WIFI_NAMESPACE  "Wifi_Store"

void my_nvs_init();

esp_err_t nvs_read_wifi_data(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz);
esp_err_t nvs_write_wifi_data(char *ssid, char *pass);




#endif