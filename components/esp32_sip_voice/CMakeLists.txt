idf_component_register(SRCS "sip_client.c" # Placeholder source file
                    INCLUDE_DIRS "include"
                    REQUIRES lwip esp_timer esp_log) # Added esp_log dependency
