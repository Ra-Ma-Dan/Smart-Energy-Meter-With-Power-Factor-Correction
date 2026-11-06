idf_component_register(SRCS "pzem_sensor.c"
                    INCLUDE_DIRS "include"
                    REQUIRES my_gpio esp_driver_uart pzem_sensor)
