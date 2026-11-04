#include <stdio.h>
#include "my_i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "my_gpio.h"


static const char *TAG = "MY I2C";

void my_i2c_init(){
    i2c_master_bus_config_t bus_cfg = {
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_PORT_NUM,
        .glicth_ignore_cnt = -7,
        .flags.enable_internal_pullup = true,
    }
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, bus_handle));

    i2c_dev_config dev_cfg = {
        .dev_addr_len = I2C_ADDR_BIT_7,
        .device_address = PMEZ_ADDRESS,
        .scl_speed = CLK_SPEED_HZ
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    ESP_LOGI(TAG, "I2C Initialized Successful");
}

void i2c_send(const int16_t *data, size_t data_len, int timeout){
    esp_err_t res = i2c_master_transmit(dev_handle, data, data_len, timeout);
    if(res != ESP_OK){
        ESP_LOGI(TAG, "Failed to transmit");
        abort();
    }

}
void i2c_send_recieve(const int16_t *data, size_t data_len, const int16_t *buffer, size_t buff_len, int timeout){
    esp_err_t res = i2c_master_transmit_recieve(dev_handle, data, data_len, buffer, buff_len, timeout);
    if(res != ESP_OK){
        ESP_LOGI(TAG, "Failed to transmit");
        abort();
    }
}
