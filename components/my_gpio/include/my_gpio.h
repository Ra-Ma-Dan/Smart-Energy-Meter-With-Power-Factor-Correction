#ifndef MY_GPIO_H
#define MY_GPIO_H

#include "driver/gpio.h"

//Capacitor banks Relays pin
#define CAPACITOR_1_RELAY   GPIO_NUM_10
#define CAPACITOR_2_RELAY   GPIO_NUM_11
#define CAPACITOR_3_RELAY   GPIO_NUM_12
#define CAPACITOR_4_RELAY   GPIO_NUM_13
#define NUM_CAP_BANKS       4

//Main power Relay
#define MAIN_POWER_RELAY    GPIO_NUM_14

//LED Indicators Pin
#define WIFI_LED_INDICATOR  GPIO_NUM_15
#define INTERNET_INDICATOR  GPIO_NUM_16
#define POFA_LED_INDICATOR  GPIO_NUM_17

//i2c pins defination
#define I2C_SDA_PIN         GPIO_NUM_21
#define I2C_SCL_PIN         GPIO_NUM_20

//UART pins configurations
#define UART_RX_PIN         GPIO_NUM_18
#define UART_TX_PIN         GPIO_NUM_19

#define PF_TARGET_MAX  0.95f   // Upper safety limit: Disengage bank if PF climbs over this (prevents over-correction)
#define MIN_LOAD_POWER 50.0f   // Minimum load in Watts before correction activates
#define PF_TARGET_MIN  0.85f   // Lower limit: Engage bank if PF falls below this

extern uint8_t active_capacitor_mask; // Bit 0 = Bank 1, Bit 1 = Bank 2, etc.

static const gpio_num_t cap_bank_gpios[NUM_CAP_BANKS] = {
    CAPACITOR_1_RELAY,
    CAPACITOR_2_RELAY,
    CAPACITOR_3_RELAY,
    CAPACITOR_4_RELAY
};

void my_gpio_init(void);

#endif