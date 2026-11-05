#ifndef MY_WIFI_H
#define MY_WIFI_H


#define WIFI_CONNECTED_BIT     BIT0
#define WIFI_DISCONNECTED_BIT  BIT1
#define AP_WIFI_AUTHMODE       WIFI_AUTH_WPA2_PSK
#define MAX_RETRY              8
#define STA_CHANNEL            0
#define STA_SCAN_METHOD        WIFI_ALL_CHANNEL_SCAN


int num_dev_connected = 0;
EventGroupHandle_t wifi_enent_handle;

void my_wifi_init();


#endif