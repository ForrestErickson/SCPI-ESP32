cmake_minimum_required(VERSION 3.10)
project(scpi_esp32_host_tests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(message_framer_test test/host/message_framer_test.cpp)
target_include_directories(message_framer_test PRIVATE src)

add_executable(framed_input_test test/host/framed_input_test.cpp)
target_include_directories(framed_input_test PRIVATE src)

add_executable(error_queue_test test/host/error_queue_test.cpp)
target_include_directories(error_queue_test PRIVATE src)

add_executable(command_registry_test test/host/command_registry_test.cpp)
target_include_directories(command_registry_test PRIVATE src)

enable_testing()
add_test(NAME message_framer_test COMMAND message_framer_test)
add_test(NAME framed_input_test COMMAND framed_input_test)
add_test(NAME error_queue_test COMMAND error_queue_test)
add_test(NAME command_registry_test COMMAND command_registry_test)

add_executable(parser_test test/host/parser_test.cpp)
target_include_directories(parser_test PRIVATE src)
add_test(NAME parser_test COMMAND parser_test)

add_executable(engine_test test/host/engine_test.cpp)
target_include_directories(engine_test PRIVATE src)
add_test(NAME engine_test COMMAND engine_test)

add_executable(identification_test test/host/identification_test.cpp)
target_include_directories(identification_test PRIVATE src)
add_test(NAME identification_test COMMAND identification_test)
