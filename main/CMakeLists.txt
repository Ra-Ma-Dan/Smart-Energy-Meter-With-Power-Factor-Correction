idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES my_wifi my_gpio my_i2c meter_http_server pzem_sensor pfc_task)
