# SmartMeter-with-PFC (ESP32 Smart Meter & Automatic Power Factor Corrector (APFC))

The design and implementation of a Smart meter that has 'Power Factor' correcting feature to help maximize efficiency in electrical energy usage.
This project implements an ESP32-based Smart Meter and Automatic Power Factor Corrector (APFC) using the ESP-IDF framework. It monitors electrical parameters via a PZEM sensor over UART, dynamically switches a 4-channel capacitor bank to correct poor power factor, and displays real-time data on an I2C LCD.

🚀 Features

• Real-time Monitoring: Measures Voltage, Current, Active Power, Energy, Frequency, and Power Factor using a PZEM-series sensor.
• Automated Correction: Features an APFC algorithm that monitors power factor and triggers a 4-channel relay connected to 4 different-sized capacitors to optimize efficiency.
• Safety Isolation: Utilizes a heavy-duty Power Relay for primary load control and system isolation.
• Local Visuals: Outputs live metrics to a 16x2 or 20x4 LCD display over the I2C bus.
• Production Ready: Built purely on ESP-IDF (v5.x recommended) leveraging FreeRTOS tasks for low-latency sampling and control loops.


🛠️ Hardware Requirements

• MCU: ESP32 Development Board
• Sensor: PZEM-004T (v3.0) or equivalent PZEM UART module + CT coil
• Capacitor Bank: 4x Capacitors (Graduated sizes, e.g., 1µF, 2.2µF, 4.7µF, 10µF depending on your target load)
• Switching Relays: 4-Channel Relay Module (for capacitor switching)
• Master Control: High-current Power Relay (for main load connection)
• Display: LCD 1602 or 2004 with an I2C backpack (PCF8574)


🔌 Pin Mapping (Default)

Component	ESP32 Pin	Description
PZEM TX	GPIO 16	UART RX2
PZEM RX	GPIO 17	UART TX2
I2C SDA (LCD)	GPIO 21	Data Line
I2C SCL (LCD)	GPIO 22	Clock Line
Relay Ch 1	GPIO 25	Capacitor 1 Switch
Relay Ch 2	GPIO 26	Capacitor 2 Switch
Relay Ch 3	GPIO 27	Capacitor 3 Switch
Relay Ch 4	GPIO 14	Capacitor 4 Switch
Main Power Relay	GPIO 12	System Load Isolation

📂 Project Structure

├── CMakeLists.txt                  # Top-level project CMake file
├── sdkconfig                       # Generated project configuration file
└── SmartMeter-with-PFC/
    ├── CMakeLists.txt              # App component CMake file
    ├── Kconfig                     # Menuconfig configuration options
    ├── main.c                      # Application entry point (FreeRTOS initialization)
    │
    ├── drivers/                    # Low-level hardware drivers (Peripheral Control)
    │   ├── gpio/
    │   │   ├── gpio_driver.c       # Low-level GPIO & Relay abstraction
    │   │   └── gpio_driver.h
    │   ├── uart/
    │   │   ├── uart_driver.c       # Low-level ESP32 UART configuration
    │   │   └── uart_driver.h
    │   └── i2c/
    │       ├── i2c_driver.c        # Low-level ESP32 I2C master bus configuration
    │       └── i2c_driver.h
    │
    └── peripherals/                # High-level component logic & application drivers
        ├── pzem/
        │   ├── pzem_sensor.c       # PZEM-004T protocol parser (uses uart_driver)
        │   └── pzem_sensor.h
        ├── lcd/
        │   ├── lcd_display.c       # PCF8574/LCD library commands (uses i2c_driver)
        │   └── lcd_display.h
        └── apfc/
            ├── apfc_system.c       # Capacitor switching and APFC logic (uses gpio_driver)
            └── apfc_system.h
             # Project documentation



⚙️ APFC Logic Overview

The firmware evaluates the target Power Factor (default threshold: 0.95).
1. Under-Correction: If the power factor drops below the threshold and the load is inductive, the system calculates the missing reactive power (VAR) and switches on the optimal combination of the 4 graduated capacitors.
2. Over-Correction: If the power factor becomes leading, the system systematically drops capacitor stages to prevent over-voltage conditions.
3. Safety Override: If critical over-current or severe low power factor faults occur, the Main Power Relay triggers to disconnect the load.

🛠️ Setup & Installation

1. Prerequisites

Ensure you have the ESP-IDF environment installed and configured:

2. Clone and Configure
```
git clone https://github.com
cd esp32-pzem-apfc
idf.py menuconfig
```
3. Build and Flash
```
idf.py build
idf.py -p [YOUR_PORT] flash monitor
```

📝 License

This project is licensed under the MIT License - see the LICENSE file for details.
