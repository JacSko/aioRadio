add_library(mcp23017_mock INTERFACE)
target_include_directories(mcp23017_mock INTERFACE
  mocks
)

add_executable(mcp23017_tests unit/mcp23017_tests.cpp
                              mocks/i2c_master_mock.cpp
                              mocks/gpio_mock.cpp
                              mocks/freertos_mock.cpp
                              ../source/mcp23017.cpp)
target_include_directories(mcp23017_tests PRIVATE
                              ../include
                              ../public
                              mocks
)
target_link_libraries(mcp23017_tests PRIVATE gtest_main
                                             gmock_main
                                             utilities
)
add_test(NAME mcp23017_tests COMMAND mcp23017_tests)
