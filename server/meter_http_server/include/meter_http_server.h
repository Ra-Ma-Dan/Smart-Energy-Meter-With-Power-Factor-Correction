#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "freertos/queue.h"
#include "pzem_sensor.h"


extern pzem_data_t live_pfc_data;
extern bool power_state;
extern float credit_balance;


// Global FreeRTOS Queue Handle
extern QueueHandle_t recharge_queue;


void start_webserver(void);




#endif