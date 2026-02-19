cmake_minimum_required(VERSION 3.20)
project(J2534Tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(j2534_core
    src/core/j2534/J2534MockApi.cpp
    src/core/diag/UdsClient.cpp
)

target_include_directories(j2534_core PUBLIC src)

add_executable(j2534_tool src/app/main.cpp)
target_link_libraries(j2534_tool PRIVATE j2534_core)

enable_testing()
