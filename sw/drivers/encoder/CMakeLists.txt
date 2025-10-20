if (NOT UNIT_TESTS)
   idf_component_register(SRCS "source/EncoderImpl.cpp"
                       INCLUDE_DIRS "include"
                                    "public"
                       REQUIRES gpio
                                )
else()
# add_subdirectory(tests)
endif()
