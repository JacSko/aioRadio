if (NOT UNIT_TESTS)
idf_component_register(SRCS "source/mcp23017.cpp"
                       INCLUDE_DIRS "include"
                                    "public"
                       REQUIRES esp_driver_gpio
                                esp_driver_i2c
                                utilities)
else()
add_subdirectory(tests)
endif()
