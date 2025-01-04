idf_component_register(SRCS "WouoUI.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver
                    PRIV_REQUIRES esp_lcd
                    PRIV_REQUIRES soc
                    PRIV_REQUIRES freertos
                    PRIV_REQUIRES ledDisplay)
