idf_component_register(SRCS "ledDisplay.cpp"
                    SRCS "ESP32-HUB75-MatrixPanel-leddrivers.cpp"
                    SRCS "ESP32-HUB75-MatrixPanel-I2S-DMA.cpp"
                    SRCS "include/platforms/esp32s3/gdma_lcd_parallel16.cpp"
                    INCLUDE_DIRS "include"
                    INCLUDE_DIRS "include/platforms/esp32s3"
                    PRIV_REQUIRES driver
                    PRIV_REQUIRES esp_lcd
                    PRIV_REQUIRES soc
                    PRIV_REQUIRES freertos
                    PRIV_REQUIRES utf8_gb2312_switch
                    PRIV_REQUIRES w25q64
                    )
