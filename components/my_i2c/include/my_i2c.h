#ifndef MY_I2C_H
#define MY_I2C_H

#include "driver/i2c_master.h"

#define CLK_SPEED_HZ    100000 //High speed mode
#define I2C_PORT_NUM    -1


extern i2c_master_bus_handle_t bus_handle;
extern i2c_master_dev_handle_t dev_handle;

void my_i2c_init();

void i2c_send(const uint8_t *data, size_t data_len, int timeout);
void i2c_send_recieve(const int8_t *data, size_t data_len, int8_t *buffer, size_t buff_len, int timeout);


#endif