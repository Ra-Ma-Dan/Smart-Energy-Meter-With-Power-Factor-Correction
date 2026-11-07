#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pzem_sensor.h"
#include "esp_log.h"
#include "my_gpio.h"

#include "meter_http_server.h"

static const char *TAG = "PZEM_SENSOR";


float simulated_energy_kwh = 0.0f;

esp_err_t pzem_init() {
    uart_config_t uart_config = {
        .baud_rate = UART_DATA_SPEED,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters
    esp_err_t err = uart_param_config(UART_PORT, &uart_config);
    if (err != ESP_OK) return err;

    // Set UART pins
    err = uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    // Install UART driver (No event queue, small ring buffer since we pull data synchronously)
    err = uart_driver_install(UART_PORT, PMEZ_FRAME_SIZE * 4, 0, 0, NULL, 0);
    return err;
}


// Standard Modbus RTU CRC-16 Calculation
static uint16_t pzem_crc16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

esp_err_t pzem_read_data(pzem_data_t *data) {
    uint8_t request[8] = {
        PMEZ_DEV_ADDR, 
        0x03,        // Read Holding Registers command
        0x00, 0x00,  // Start address 0x0000
        0x00, 0x0A,  // Read 10 registers (20 bytes of data)
        0x00, 0x00   // CRC placeholders
    };

    // Calculate CRC for the request
    uint16_t req_crc = pzem_crc16(request, 6);
    request[6] = req_crc & 0xFF;         // CRC Low
    request[7] = (req_crc >> 8) & 0xFF;  // CRC High

    // Clear RX buffer before transmission
    uart_flush_input(UART_PORT);

    // Write command out to the PZEM
    uart_write_bytes(UART_PORT, (const char *)request, 8);

    // Expected frame size = 1 byte (addr) + 1 byte (cmd) + 1 byte (bytes count) + 20 bytes (data) + 2 bytes (crc) = 25 bytes
    uint8_t response[25];
    int len = uart_read_bytes(UART_PORT, response, sizeof(response), pdMS_TO_TICKS(500));

    if (len < (int)sizeof(response)) {
        ESP_LOGW(TAG, "Timeout or incomplete response received. Got %d bytes", len);
        return ESP_ERR_TIMEOUT;
    }

    // Verify CRC
    uint16_t resp_crc = pzem_crc16(response, sizeof(response) - 2);
    uint16_t received_crc = response[sizeof(response) - 2] | (response[sizeof(response) - 1] << 8);

    if (resp_crc != received_crc) {
        ESP_LOGE(TAG, "CRC Error! Expected: 0x%04X, Got: 0x%04X", resp_crc, received_crc);
        return ESP_FAIL;
    }

    // Parse bytes (Modbus elements are Big-Endian, PZEM elements spread across pairs)
    // Data payload starts at index 3
    uint32_t raw_val;

    // Voltage (Reg 0x00) -> 0.1V Resolution
    data->voltage = ((response[3] << 8) | response[4]) * 0.1f;

    // Current (Reg 0x01-0x02) -> 0.001A Resolution
    raw_val = ((uint32_t)response[5] << 24) | ((uint32_t)response[6] << 16) | ((uint32_t)response[7] << 8) | response[8];
    data->current = raw_val * 0.001f;

    // Power (Reg 0x03-0x04) -> 0.1W Resolution
    raw_val = ((uint32_t)response[9] << 24) | ((uint32_t)response[10] << 16) | ((uint32_t)response[11] << 8) | response[12];
    data->power = raw_val * 0.1f;

    // Energy (Reg 0x05-0x06) -> 1Wh Resolution
    raw_val = ((uint32_t)response[13] << 24) | ((uint32_t)response[14] << 16) | ((uint32_t)response[15] << 8) | response[16];
    data->energy = (float)raw_val;

    // Frequency (Reg 0x07) -> 0.1Hz Resolution
    data->frequency = ((response[17] << 8) | response[18]) * 0.1f;

    // Power Factor (Reg 0x08) -> 0.01 Resolution
    data->power_factor = ((response[19] << 8) | response[20]) * 0.01f;

    // Alarm status (Reg 0x09)
    data->alarms = (response[21] << 8) | response[22];

    return ESP_OK;
}

esp_err_t pzem_reset_energy() {
    uint8_t command[4] = {
        PMEZ_DEV_ADDR,
        0x42,        // PZEM-004T custom Modbus code for energy reset
        0x00, 0x00   // CRC placeholders
    };

    uint16_t crc = pzem_crc16(command, 2);
    command[2] = crc & 0xFF;
    command[3] = (crc >> 8) & 0xFF;

    uart_flush_input(UART_PORT);
    uart_write_bytes(UART_PORT, (const char *)command, 4);

    uint8_t response[4];
    int len = uart_read_bytes(UART_PORT, response, sizeof(response), pdMS_TO_TICKS(200));

    if (len == 4 && pzem_crc16(response, 2) == (response[2] | (response[3] << 8))) {
        ESP_LOGI(TAG, "Energy counters reset successful.");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to reset energy counters.");
    return ESP_FAIL;
}


float generate_random_num(float start, float end) {
    // 1. Get a random fraction between 0.0 and 1.0
    float scale = (float)rand() / (float)RAND_MAX; 

    // 2. Scale it to fit your specific [start, end] range
    return start + (scale * (end - start));
}



esp_err_t random_pzem_reading(pzem_data_t *data) {
    if (data == NULL) return ESP_ERR_INVALID_ARG;

    // 1. Generate core physical values
    float volt = generate_random_num(220.0f, 245.0f);
    float curr = (power_state) ? generate_random_num(15.0f, 30.0f) : 0.00f;
    float pf   = generate_random_num(0.75f, 0.99f); // Real world loads rarely hit absolute 0.0
    float freq = generate_random_num(49.5f, 50.5f); // Assuming standard 50Hz grid

    // 2. Calculate physically consistent Active Power (Watts)
    float active_power = volt * curr * pf;

    // 3. Simulate Active Energy Accumulation (Wh or kWh depending on your struct)
    float simulated_time_hours = 2.0f / 3600.0f; // Simulating 2 seconds passing
    float generated_wh = active_power * simulated_time_hours;
    simulated_energy_kwh += (generated_wh / 1000.0f); // Convert Wh to kWh

    // 4. Assign to struct
    data->voltage = volt;
    data->current = curr;
    data->power_factor = pf;
    data->frequency = freq;
    data->power = active_power; 
    data->energy = simulated_energy_kwh; 

    return ESP_OK;
}
