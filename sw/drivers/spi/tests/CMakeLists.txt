add_executable(SPIDriverTests unit/SPIDriverTests.cpp
                              ../source/SPIDriver.cpp
)
target_include_directories(SPIDriverTests PUBLIC ../include
                                                 ../public
                                                 ../../mcp23017/public/
)
target_link_libraries(SPIDriverTests PRIVATE gtest_main
                                             gmock_main
                                             espFrameworkMocks
                                             config
                                             GPIOMocks
                                             utilities
)
add_test(NAME SPIDriverTests COMMAND SPIDriverTests)

