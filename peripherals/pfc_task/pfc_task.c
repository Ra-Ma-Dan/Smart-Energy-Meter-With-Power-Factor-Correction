#include <stdio.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pfc_task.h"
#include "my_gpio.h"

void power_factor_correction_task(void *pvParameters) {

    while (1) {
        float P = live_pfc_data.power;
        float pf = live_pfc_data.power_factor;

        // Skip calculations if line load is completely idle or power factor is already high/leading
        if (P < 50.0f || pf >= 0.98f || pf <= 0.0f) {
            if (active_capacitor_mask != 0) {
                for (int i = 0; i < NUM_CAP_BANKS; i++) gpio_set_level(cap_bank_gpios[i], 0);
                active_capacitor_mask = 0;
                ESP_LOGI("PFC", "Load idle. All capacitor banks dropped offline.");
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // 1. Calculate the current inductive Apparent Power (VA) and Reactive Power (VAR)
        float S = P / pf; 
        float Q_current = sqrtf((S * S) - (P * P)); // Missing inductive VAR on the line

        // 2. Binary combinatorial optimization: Test all 16 permutations to find the optimal combo
        int best_mask = 0;
        float minimum_var_error = 99999.0f;

        for (int mask = 0; mask < 16; mask++) {
            float total_combination_var = 0;
            for (int i = 0; i < NUM_CAP_BANKS; i++) {
                if (mask & (1 << i)) {
                    total_combination_var += bank_var_values[i];
                }
            }

            // We want to offset Q_current as close as possible without over-correcting significantly
            float residual_error = Q_current - total_combination_var;
            
            // Apply slight penalty if combination creates a leading PF (over-correction), but look for closest absolute error
            if (residual_error < 0) residual_error = fabsf(residual_error) * 1.5f; 

            if (residual_error < minimum_var_error) {
                minimum_var_error = residual_error;
                best_mask = mask;
            }
        }

        // 3. Execution: If optimal combination changes, commit it to the output relays
        if (best_mask != active_capacitor_mask) {
            ESP_LOGI("PFC", "Optimizing Capacitors. Target Deficit: %.1f VAR. Selecting Combination Mask: 0x%X", Q_current, best_mask);
            
            for (int i = 0; i < NUM_CAP_BANKS; i++) {
                bool engage = (best_mask & (1 << i)) ? true : false;
                gpio_set_level(cap_bank_gpios[i], engage ? 1 : 0);
            }
            active_capacitor_mask = best_mask;

            // Wait 3.5 seconds to settle line voltage transients before inspecting telemetry again
            vTaskDelay(pdMS_TO_TICKS(3500));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

