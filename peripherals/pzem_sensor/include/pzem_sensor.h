#ifndef PMEZ_SENSOR_H
#define PMEZ_SENSOR_H

#include "driver/uart.h"

#define UART_DATA_SPEED 9600
#define UART_PORT       UART_NUM_0
#define PMEZ_FRAME_SIZE 128  // Standard PM sensor frame length is 32
#define PMEZ_DEV_ADDR   0x01


extern float simulated_energy_kwh;

/**
 * @brief Structure to hold parsed sensor readings from the PZEM-004T
 */
typedef struct {
    float voltage;       /**< Voltage in Volts (V) */
    float current;       /**< Current in Amperes (A) */
    float power;         /**< Active Power in Watts (W) */
    float energy;        /**< Energy consumption in Watt-hours (Wh) */
    float frequency;     /**< Line frequency in Hertz (Hz) */
    float power_factor;  /**< Power factor (0.00 to 1.00) */
    uint16_t alarms;     /**< Alarm status (0xFFFF if active) */
} pzem_data_t;


/**
 * @brief Initializes the UART driver for the PZEM-004T sensor
 * 
 * @return esp_err_t ESP_OK on success, or an error code
 */
esp_err_t pzem_init();

/**
 * @brief Reads all electrical parameters from the PZEM-004T sensor
 * 
 * @param data Pointer to the struct where results will be stored
 * @return esp_err_t ESP_OK on success, ESP_FAIL on CRC/parsing failure, or ESP_ERR_TIMEOUT
 */


esp_err_t pzem_read_data(pzem_data_t *data);

/**
 * @brief Resets the accumulated energy value on the sensor to 0
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t pzem_reset_energy();

esp_err_t random_pzem_reading(pzem_data_t *data);


#endif