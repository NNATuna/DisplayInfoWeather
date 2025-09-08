idf_component_register(SRCS "displayinfoweather.c"
                       PRIV_REQUIRES esp_wifi nvs_flash esp_http_server esp_http_client json
                       REQUIRES esp_driver_i2c u8g2 u8g2-hal-esp-idf
                       INCLUDE_DIRS ".")
