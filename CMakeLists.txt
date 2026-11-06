cmake_minimum_required(VERSION 3.10)
project(scpi_esp32_host_tests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(message_framer_test test/host/message_framer_test.cpp)
target_include_directories(message_framer_test PRIVATE src)

enable_testing()
add_test(NAME message_framer_test COMMAND message_framer_test)
