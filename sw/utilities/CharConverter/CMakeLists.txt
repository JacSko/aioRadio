if (NOT UNIT_TESTS)

   idf_component_register(SRCS "source/CharConverter.cpp"
                       INCLUDE_DIRS "public"
                       PRIV_INCLUDE_DIRS "include"
                       PRIV_REQUIRES utilities)

endif()
