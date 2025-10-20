cmake_minimum_required(VERSION 3.16)

set(SUPPORTED_TARGETS "esp32")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(AIO_COMPONENT_DIRECTORIES "sw/main"
                              "sw/config"
                              "sw/control"
                              "sw/display"
                              "sw/drivers"
                              "sw/drivers/gpio"
                              "sw/drivers/encoder"
                              "sw/drivers/mcp23017"
                              "sw/drivers/spi"
                              "sw/tuner"
                              "sw/audio"
                              "sw/utilities"
                              "sw/utilities/CharConverter")

if(UNIT_TESTS)
   project(aioRadioUT CXX)
   message(STATUS "Building unit tests")
   enable_testing()
   add_subdirectory(external/googletest)
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
   add_definitions(-DUNIT_TESTS)
   foreach(directory ${AIO_COMPONENT_DIRECTORIES})
      add_subdirectory(${directory})
   endforeach()
elseif("${IDF_TARGET}" IN_LIST SUPPORTED_TARGETS)
   list(APPEND EXTRA_COMPONENT_DIRS ${AIO_COMPONENT_DIRECTORIES})
   include($ENV{IDF_PATH}/tools/cmake/project.cmake)
   idf_build_set_property(MINIMAL_BUILD ON)
   project(aioRadio CXX)
else()
   message(ERROR "Unknown target [${TARGET}]")
endif()
