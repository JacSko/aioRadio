add_executable(TableConverterTests TableConverterTests.cpp)
target_include_directories(TableConverterTests PRIVATE ../../public)
target_link_libraries(TableConverterTests PRIVATE gtest gtest_main)
add_test(NAME TableConverterTests COMMAND TableConverterTests)
