add_library(espFrameworkMocks STATIC
   freertos_mock.cpp
   i2c_master_mock.cpp
   spi_master_mock.cpp
   gpio_mock.cpp
)

target_include_directories(espFrameworkMocks PUBLIC
   .
)

target_link_libraries(espFrameworkMocks PUBLIC
   gmock_main
)
