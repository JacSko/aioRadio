if (NOT UNIT_TESTS)
   idf_component_register(SRCS "source/IAudioControlImpl.cpp"
                       INCLUDE_DIRS "public"
                       PRIV_INCLUDE_DIRS "include"
                       REQUIRES tuner)
endif()
