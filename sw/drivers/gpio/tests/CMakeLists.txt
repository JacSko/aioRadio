add_library(GPIOMocks INTERFACE)
target_include_directories(GPIOMocks INTERFACE ../public mocks/)



add_executable(socGpioTests unit/socgpio_tests.cpp
                            ../source/socGPIO.cpp
)
target_include_directories(socGpioTests PRIVATE ../include
                                                ../public
)
target_link_libraries(socGpioTests PRIVATE gtest_main
                                           gmock_main
                                           espFrameworkMocks
                                           utilities
                                           config
)
add_test(NAME socGpioTests COMMAND socGpioTests)


add_executable(i2cGpioTests unit/i2cgpio_tests.cpp
                            ../source/i2cGPIO.cpp
)
target_include_directories(i2cGpioTests PRIVATE ../include
                                                ../public
                                                ../../mcp23017/public/
)
target_link_libraries(i2cGpioTests PRIVATE gtest_main
                                           gmock_main
                                           espFrameworkMocks
                                           utilities
                                           config
                                           mcp23017_mock
)
add_test(NAME i2cGpioTests COMMAND i2cGpioTests)
