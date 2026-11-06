#include "my_gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "GPIO DRIVER";
uint8_t active_capacitor_mask = 0;

void my_gpio_init(void){
    gpio_config_t pins_cfg = {
        .pin_bit_mask = (1ULL << CAPACITOR_1_RELAY) |
                        (1ULL << CAPACITOR_2_RELAY) |
                        (1ULL << CAPACITOR_3_RELAY) |
                        (1ULL << CAPACITOR_4_RELAY) |
                        (1ULL << POFA_LED_INDICATOR)|
                        (1ULL << WIFI_LED_INDICATOR)|
                        (1ULL << INTERNET_INDICATOR)|
                        (1ULL << MAIN_POWER_RELAY),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    esp_err_t res = gpio_config(&pins_cfg);
    if (res != ESP_OK){
        ESP_LOGE(TAG, "Configuration failed: %s", esp_err_to_name(res));
        abort();
    }
    for (int i = 0; i < NUM_CAP_BANKS; i++) {
        gpio_set_level(cap_bank_gpios[i], 0); // Start completely disengaged
    }

    gpio_config_t lcd_ctr_pin = {
        .pin_bit_mask = (1ULL << LCD_CNT_BTN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    res = gpio_config(&lcd_ctr_pin);
    if (res != ESP_OK){
        ESP_LOGE(TAG, "Configuration failed: %s", esp_err_to_name(res));
        abort();
    }
    ESP_LOGI(TAG, "Initialization Succeded");
}

