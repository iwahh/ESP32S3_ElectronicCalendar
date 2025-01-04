idf_component_register(SRCS "dht11.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES freertos
                    PRIV_REQUIRES driver
                    PRIV_REQUIRES soc
                    PRIV_REQUIRES hal
                    PRIV_REQUIRES esp_rom
                    )
