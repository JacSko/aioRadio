if (NOT UNIT_TESTS)

idf_component_register(SRCS "driver/source/DisplayDriver.cpp"
                            "gui/source/MainGUIImpl.cpp"
                            "gui/fonts/montserrat_ebu_latin_14.c"
                            "gui/fonts/montserrat_ebu_latin_18.c"
                            "gui/fonts/montserrat_ebu_latin_24.c"
                       PRIV_REQUIRES esp_driver_spi
                                     esp_lcd_st7796
                                     utilities
                                     gpio
                       INCLUDE_DIRS "gui/public"
                                    "driver/public"
                       REQUIRES esp_lvgl_port
                       PRIV_INCLUDE_DIRS "gui/include")


endif()
