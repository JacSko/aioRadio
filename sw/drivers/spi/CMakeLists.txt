if (NOT UNIT_TESTS)

   idf_component_register(SRCS "source/SPIDriver.cpp"
                       INCLUDE_DIRS "public"
                       REQUIRES esp_driver_spi
                                utilities
                                gpio)
else()
add_subdirectory(tests)
endif()
