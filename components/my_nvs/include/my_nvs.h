#ifndef MY_NVS_H
#define MY_NVS_H

#define NVS_WIFI_NAMESPACE  "Wifi_Store"

void my_nvs_init();

void nvs_read_wifi_data(const char *ssid, size_t ssid_sz, char *pass, size_t pass_sz);
void nvs_write_wifi_data(char *ssid, char *pass);




#endif