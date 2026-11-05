#ifndef PMEZ_SENSOR_H
#define PMEZ_SENSOR_H

#include "driver/uart.h"

#define UART_DATA_SPEED 9600
#define UART_PORT       UART_PORT_0
#define PMEZ_FRAME_SIZE 32  // Standard PM sensor frame length
#define PMEZ_DEV_ADDR   0x01

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

// /**
//  * @brief Configuration struct for the PZEM-004T sensor
//  */
// typedef struct {
//     uart_port_t uart_port;     /**< UART port to use (e.g., UART_NUM_1) */
//     int tx_pin;                /**< GPIO number for ESP TX -> PZEM RX */
//     int rx_pin;                /**< GPIO number for ESP RX -> PZEM TX */
//     uint8_t addr;              /**< Modbus address (Default is usually 0x01) */
// } pzem_config_t;


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

esp_err_t random_pzem_reading();
#endif // PZEM004T_H