#include <stdio.h>
#include "lcd_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "meter_http_server.h"

#include "my_gpio.h"
#include "my_i2c.h"


static const char *TAG = "LCD_DISPLAY";

void send_nibble(uint8_t nibble, uint8_t mode){
  // Combines the nibble data with the global active backlight state
  uint8_t data = (nibble & 0xF0) | active_backlight | mode;

  uint8_t transactions[2] = {
    data | LCD_ENABLE,
    data & ~LCD_ENABLE
  };

  i2c_send((const uint8_t *)transactions, sizeof(transactions), portTICK_PERIOD_MS);
  vTaskDelay(pdMS_TO_TICKS(2));
}



void send_byte(uint8_t val, uint8_t mode){
    uint8_t high_nibble = (val & 0xF0);
    uint8_t low_nibble  = ((val << 4) & 0xF0);
    
    // printf("High nibble %d\n", high_nibble);
    // printf("Low nibble %d\n", low_nibble);
    
  send_nibble(high_nibble, mode);
  send_nibble(low_nibble, mode);
}

void lcd_clear() {
    send_byte(0x01, 0); // Clear Display Command
    vTaskDelay(pdMS_TO_TICKS(5));
}    

void init_lcd_display(){
  vTaskDelay(pdMS_TO_TICKS(50));

  send_nibble(0x30, 0);
  vTaskDelay(pdMS_TO_TICKS(5));
  send_nibble(0x30, 0);
  vTaskDelay(pdMS_TO_TICKS(1));
  send_nibble(0x30, 0);

  send_nibble(0x20, 0); //Force LCD to 4-bit mode

  send_byte(0x28, 0);
  send_byte(0x0C, 0); //Display ON, Cursor OFF
  send_byte(0x01, 0); //Clear Display
  vTaskDelay(pdMS_TO_TICKS(5));
  send_byte(0x06, 0); //Cursor move direction

  ESP_LOGI(TAG, "Initialization Succeeded");
}

void set_cursor(uint8_t row, uint8_t col){
  uint8_t address = (row == 0) ? (0x00 + col) : (0x40 + col);
  send_byte(0x80 | address, 0);
}
void send_word(const char *word){
  while(*word){
    send_byte(*word++, LCD_REG_SELECT);
  }
}


void lcd_display_task(void *pvParameters) {
    init_lcd_display();
    last_activity_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    //last_activity_time = xTaskCheckForTimeOut(NULL, NULL); // Initialize timer
    
    uint8_t display_page = 0;
    uint32_t last_rotation_time = 0;
    char line1[17];
    char line2[17];

    while(1) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // --- 1. HANDLE WAKE BUTTON ---
        if (gpio_get_level(LCD_CNT_BTN) == 0) { 
            // Debounce delay
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(LCD_CNT_BTN) == 0) {
                last_activity_time = current_time; // Reset sleep timer
                
                if (!display_awake) {
                    active_backlight = LCD_BACKLIGHT_ON_VALUE;         // Turn backlight variable ON
                    display_awake = true;
                    send_byte(0x0C, 0);           // Wake up LCD command (Display ON)
                    lcd_clear();
                }    
                
                // Wait for button release to avoid rapid firing
                while(gpio_get_level(LCD_CNT_BTN) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                }    
            }    
        }    

        // --- 2. HANDLE SLEEP TIMEOUT ---
        if (display_awake && (current_time - last_activity_time >= SLEEP_TIMEOUT_MS)) {
            active_backlight = 0x00;                 // Turn backlight variable OFF
            display_awake = false;
            send_byte(0x08, 0);                   // Send LCD display OFF command to save power
            lcd_clear();
        }    

        // --- 3. HANDLE PARAMETER ROTATION (Only when awake) ---
        if (display_awake) {
            if (current_time - last_rotation_time >= ROTATION_DELAY_MS) {
                last_rotation_time = current_time;
                lcd_clear();

                switch (display_page) {
                    case 0: // Pair 1: Voltage and Current
                        snprintf(line1, sizeof(line1), "Volt: %.1f V", live_pfc_data.voltage);
                        snprintf(line2, sizeof(line2), "Curr: %.2f A", live_pfc_data.current);
                        display_page = 1;
                        break;

                    case 1: // Pair 2: Active Power and Accumulated Energy    
                        snprintf(line1, sizeof(line1), "Power: %.1f W", live_pfc_data.power);
                        snprintf(line2, sizeof(line2), "Energy: %.1fkWh", live_pfc_data.energy);
                        display_page = 2; // Loop back to the first pair
                        break;

                    case 2: // Pair 3: Current Balance and power factor    
                        snprintf(line1, sizeof(line1), "Bal: %.1f W", credit_balance);
                        snprintf(line2, sizeof(line2), "freq: %.1fkWh", live_pfc_data.frequency);
                        display_page = 0; // Loop back to the first pair
                        break;
                }        

                // Render page content onto the screen
                set_cursor(0, 0);
                send_word(line1);
                set_cursor(1, 0);
                send_word(line2);
                
            }    
        }    

        // Fast polling rate to keep the button highly responsive
        vTaskDelay(pdMS_TO_TICKS(100));
    }    
} 
