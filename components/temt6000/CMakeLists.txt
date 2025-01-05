idf_component_register(SRCS "temt6000.cpp" 
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver
                    PRIV_REQUIRES hal
                    PRIV_REQUIRES log
                    PRIV_REQUIRES esp_adc)
