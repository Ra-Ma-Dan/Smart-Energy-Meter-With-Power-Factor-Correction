#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#define LCD_ADDR 0x27

#define LCD_REG_SELECT 0b00000001
#define LCD_ENABLE     0b00000100
#define LCD_BACKLIGHT_ON_VALUE  0x08  // 1. Define a fixed macro for the default value (Give it a unique name)
#define SLEEP_TIMEOUT_MS  (2 * 60 * 1000)  // 2 minutes sleep timeout (change to 5 * 60 * 1000 for 5 mins)
#define ROTATION_DELAY_MS (4000)            // Change parameters every 4 seconds

// Global tracking variables
static uint32_t last_activity_time = 0;
static bool display_awake = true;


// 2. Create the changeable state variable used by your send_nibble function
static uint8_t active_backlight = LCD_BACKLIGHT_ON_VALUE;  // Keep your existing backlight flag value (usually 0x08 for I2C bags)


void init_lcd_display();
void lcd_display_task(void *pvParameters);

#endif




