idf_component_register(SRCS "my_wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_netif nvs_flash esp_wifi esp_event meter_http_server)
