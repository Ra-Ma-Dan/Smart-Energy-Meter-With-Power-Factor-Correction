#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/queue.h"

#include "pzem_sensor.h"
#include "my_gpio.h"
#include "my_i2c.h"
#include "my_wifi.h"
#include "meter_http_server.h"
#include "pfc_task.h"
#include "lcd_display.h"

static const char *TAG = "App Main";
QueueHandle_t recharge_queue = NULL;
pzem_data_t live_pfc_data;
//1 Read Parameters form Sensor
void energy_sensor_task(void *pvParameters) {
    pzem_data_t readings;

    while (1) {
        //if (pzem_read_data(&readings) == ESP_OK) {
        if (random_pzem_reading(&readings) == ESP_OK) {

            live_pfc_data = readings;
            
            // Optional: Calculate energy metrics consumption drop rates
            // If active load consumption exists, draw against balance reserves
            if (readings.power > 0.1f) {
                // Approximate cost deduction per second based on current load
                // e.g., ₦50 per kWh -> (Power in W / 1000) * (Hours) * Cost
                float cost_per_sec = (readings.power / 1000.0f) * (1.0f / 3600.0f) * 50.0f;
                if (credit_balance > cost_per_sec) {
                    credit_balance -= cost_per_sec;
                } else {
                    credit_balance = 0.0f;
                }
            }
        } else {
            ESP_LOGW(TAG, "Telemetry link dropped. Re-checking device bus...");
        }
        vTaskDelay(pdMS_TO_TICKS(3000)); //3 secs between readings. to be changed for real PZEM sensor
    }
}

// 2. Hardware Relay Protection Safety Interlock Task
void power_task(void *pvParameters) {

    while (1) {
        // Auto-trip power cut if balance hits critical threshold
        // User is only authorized to turn power ON if they actually have active credit
        bool clear_to_run = (credit_balance > 3.0f) && power_state;
        
        gpio_set_level(MAIN_POWER_RELAY, clear_to_run ? 1 : 0);

        // If credit runs out mid-operation, sync the user's state flag back to false
        if (credit_balance <= 3.0f && power_state) {
            power_state = false;
            ESP_LOGW(TAG, "Credit balance depleted! Automatic relay shutdown triggered.");
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms safety monitoring window
    }
}
// 3. Extensible Energy Logic Hooks Tasks
void low_credit_task(void *pvParameters) {
    while (1) {
        // Safe check for absolute low balance conditions
        if (credit_balance <= 20.0f && credit_balance > 3.0f) {
            ESP_LOGW(TAG, "Low Credit Alert! Remaining Balance: ₦%.2f", credit_balance);
            // Optional: Toggle a low-credit alert LED pin or beeper sequence here
        }
        
        // Check balance status once every 5 seconds to reduce system processing overhead
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


//4 The recharge task that processes the credits
void recharge_task(void *pvParameters) {
    recharge_queue = xQueueCreate(5, sizeof(float));
    if (recharge_queue == NULL) {
        ESP_LOGE(TAG, "Failed to initialize standard FreeRTOS queue tracking system");
        return;
    }
    float received_topup = 0.0f;
    ESP_LOGI(TAG, "Recharge background task handling initialized.");

    while (1) {
        // Block indefinitely (portMAX_DELAY) until a recharge arrives from the HTTP thread
        if (xQueueReceive(recharge_queue, &received_topup, portMAX_DELAY) == pdPASS) {
            
            // 1. Thread-safe application of the balance adjustment
            credit_balance += received_topup;
            ESP_LOGI(TAG, "[TASK] Balance credited successfully. New Total: ₦%.2f", credit_balance);
            
            // 2. Clear critical load-shedding interlocks if necessary
            // As seen in your frontend JS script, if credit drops <= 3.0, the load drops.
            // If this recharge brings them back into positive standing, activate the relay.

            // 3. (Optional but Recommended) Commit to flash memory here
            // save_balance_to_nvs(credit_balance);
        }
    }
}


void app_main(void)
{
    my_wifi_init();
    my_i2c_init();
    my_gpio_init();
    
    // if (pzem_init() != ESP_OK){
    //     ESP_LOGI(TAG, "Failed to initialized PZEM sensor");
    // }

    xTaskCreatePinnedToCore(power_factor_correction_task, "Power_Factor_Correction", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(energy_sensor_task, "Energy_Sensor_Task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(power_task, "Powering_ON_OFF_Task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(recharge_task, "Recharging_task", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(low_credit_task, "Low Credit Task", 2048, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(lcd_display_task, "LCD_DISPLAY_TASK", 4096, NULL, 6, NULL, 0);
     
    while(1){

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}
