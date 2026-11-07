idf_component_register(SRCS "meter_http_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server pzem_sensor my_nvs
                    EMBED_TXTFILES "../webpage/index.html"
                                   "../webpage/scripts.js"
                                   "../webpage/style.css")
