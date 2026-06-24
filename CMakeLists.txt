cmake_minimum_required(VERSION 3.20)
project(J2534Tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(j2534_core
    src/core/j2534/J2534MockApi.cpp
    src/core/j2534/J2534Error.cpp
    src/core/diag/NrcDecoder.cpp
    src/core/diag/UdsClient.cpp
    src/core/diag/AsyncUdsClient.cpp
    src/core/transport/IsoTpEngine.cpp
)

if (WIN32)
    target_sources(j2534_core PRIVATE src/core/j2534/J2534DynamicApi.cpp)
endif()

target_include_directories(j2534_core PUBLIC src)

add_executable(j2534_tool src/app/main.cpp)
target_link_libraries(j2534_tool PRIVATE j2534_core)

add_executable(test_uds src/tests/test_uds.cpp)
target_link_libraries(test_uds PRIVATE j2534_core)

add_executable(test_isotp src/tests/test_isotp.cpp)
target_link_libraries(test_isotp PRIVATE j2534_core)

add_executable(test_async_uds src/tests/test_async_uds.cpp)
target_link_libraries(test_async_uds PRIVATE j2534_core)

enable_testing()
add_test(NAME uds_tests COMMAND test_uds)
add_test(NAME isotp_tests COMMAND test_isotp)
add_test(NAME async_uds_tests COMMAND test_async_uds)
