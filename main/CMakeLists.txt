idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp32_sip_voice audio_hal codec esp_wifi esp_event nvs_flash lwip)
