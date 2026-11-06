#include <stdio.h>
#include "my_i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "my_gpio.h"
#include "lcd_display.h"

static const char *TAG = "MY I2C";

i2c_master_bus_handle_t bus_handle = NULL;
i2c_master_dev_handle_t dev_handle = NULL;

void my_i2c_init(){
    i2c_master_bus_config_t bus_cfg = {
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_PORT_NUM,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        }
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = CLK_SPEED_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    ESP_LOGI(TAG, "I2C Initialized Successful");
}

void i2c_send(const uint8_t *data, size_t data_len, int timeout){
    // Explicitly casting 'data' from (const int8_t*) to (const uint8_t*) to fix signedness
    esp_err_t res = i2c_master_transmit(dev_handle, (const uint8_t *)data, data_len, timeout);
    if(res != ESP_OK){
        ESP_LOGI(TAG, "Failed to transmit");
        abort();
    }
}

void i2c_send_recieve(const int8_t *data, size_t data_len, int8_t *buffer, size_t buff_len, int timeout){
    // 1. Removed 'const' from 'int8_t *buffer' in the function arguments above
    // 2. Cast 'data' to (const uint8_t*) and 'buffer' to (uint8_t*) below
    esp_err_t res = i2c_master_transmit_receive(dev_handle, (const uint8_t *)data, data_len, (uint8_t *)buffer, buff_len, timeout);
    if(res != ESP_OK){
        ESP_LOGI(TAG, "Failed to transmit_recieve");
        abort();
    }
}

