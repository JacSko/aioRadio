if (NOT UNIT_TESTS)
idf_component_register(SRCS "devicedriver/source/FirmwareLoader.cpp"
                            "devicedriver/source/DeviceDriverImpl.cpp"
                            "devicedriver/source/TunerApiImpl.cpp"
                            "dab/source/DABTunerImpl.cpp"
                            "fm/source/FMTunerImpl.cpp"
                            "manager/source/TunerManager.cpp"
                       INCLUDE_DIRS "devicedriver/public"
                                    "dab/public"
                                    "fm/public"
                                    "manager/public"
                                    "public"
                       PRIV_INCLUDE_DIRS "devicedriver/include"
                                         "devicedriver/firmware"
                                         "dab/include"
                                         "fm/include"
                       REQUIRES spi
                                utilities
                                audio
                                CharConverter
                                display)
endif()
