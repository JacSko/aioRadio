if (NOT UNIT_TESTS)

   idf_component_register(SRCS "source/UserControl.cpp"
                       PRIV_REQUIRES
                                     utilities
                                     display
                                     config
                                     encoder
                                     audio
                       INCLUDE_DIRS "public"
                       )


endif()
