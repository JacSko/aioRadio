if (NOT UNIT_TESTS)

idf_component_register(SRCS ""
                       INCLUDE_DIRS "public")

else()
   add_library(utilities INTERFACE)
   target_include_directories(utilities INTERFACE "public/")
   add_subdirectory(tests/unit)
endif()
