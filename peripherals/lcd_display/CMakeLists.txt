idf_component_register(SRCS "lcd_display.c"
                    INCLUDE_DIRS "include"
                    REQUIRES my_gpio my_i2c meter_http_server)
