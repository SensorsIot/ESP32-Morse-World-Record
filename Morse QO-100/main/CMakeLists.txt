idf_component_register(SRCS "main.c" "main" "radios"
                    INCLUDE_DIRS "."
                    REQUIRES esp_phy console bt esp_wifi nvs_flash esp_timer driver)
