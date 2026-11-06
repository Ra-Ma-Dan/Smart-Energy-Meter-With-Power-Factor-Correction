#ifndef MY_WIFI_H
#define MY_WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


#define WIFI_CONNECTED_BIT     BIT0
#define WIFI_DISCONNECTED_BIT  BIT1
#define AP_WIFI_AUTHMODE       WIFI_AUTH_WPA2_PSK
#define STA_SCAN_METHOD        WIFI_ALL_CHANNEL_SCAN
#define MAX_RETRY              8
#define STA_CHANNEL            0

// extern avoids compilation errors if included in multiple .c source files
extern int num_dev_connected;
extern EventGroupHandle_t wifi_enent_handle;

void my_wifi_init(void);


#endif // MY_WIFI_H
