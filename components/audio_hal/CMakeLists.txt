idf_component_register(SRCS "audio_hal.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_i2s) # ESP-IDF v5.x, for v4.x it might be just "driver" or specific i2s component name
