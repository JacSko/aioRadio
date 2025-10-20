if (NOT UNIT_TESTS)
idf_component_register(SRCS "source/socGPIO.cpp"
                            "source/i2cGPIO.cpp"
                            "source/GPIOProvider.cpp"
                       INCLUDE_DIRS "include"
                                    "public"
                       REQUIRES esp_driver_gpio
                                utilities
                                config
                                mcp23017)
else()
add_subdirectory(tests)
endif()
