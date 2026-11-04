#ifndef MY_WIFI_H
#define MY_WIFI_H


#define AP_STACONNECTED_BIT     BIT0
#define AP_STADISCONNECTED_BIT  BIT0
#define AP_WIFI_AUTHMODE        WIFI_AUTH_WPA2_PSK

int num_dev_connected = 0;

void my_wifi_init();


#endif