# Optional host-side tests (not part of the firmware build).
#   cmake -S host -B build-host && cmake --build build-host && ./build-host/oob_protocol_test
cmake_minimum_required(VERSION 3.13)
project(modxo_host_tests C)

add_executable(oob_protocol_test
    oob_protocol_test.c
    ${CMAKE_CURRENT_LIST_DIR}/../modxo_modules/oob_debug/oob_protocol.c
)

target_include_directories(oob_protocol_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../modxo_modules/oob_debug/include
)

target_compile_definitions(oob_protocol_test PRIVATE OOB_HOST_TEST=1)
