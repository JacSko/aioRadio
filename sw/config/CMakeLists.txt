if (NOT UNIT_TESTS)

idf_component_register(SRCS ""
                       INCLUDE_DIRS ".")

else()
   add_library(config INTERFACE)
   target_include_directories(config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
