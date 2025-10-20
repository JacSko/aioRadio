if (NOT UNIT_TESTS)
idf_component_register(SRCS ""
                       REQUIRES gpio
                                mcp23017
                                encoder
                                spi)

else()
   add_subdirectory(tests/mocks)
endif()
