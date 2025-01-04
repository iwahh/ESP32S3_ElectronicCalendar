idf_component_register(SRCS "w25q64.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver
                    PRIV_REQUIRES freertos
                    PRIV_REQUIRES hal
                    PRIV_REQUIRES soc
                    PRIV_REQUIRES utf8_gb2312_switch
                    )
