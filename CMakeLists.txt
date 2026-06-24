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
    src/core/logging/TraceLogger.cpp
    src/core/automation/ScriptRunner.cpp
    src/core/ui/DiagnosticController.cpp
    src/core/j2534/DeviceValidator.cpp
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

add_executable(test_script_runner src/tests/test_script_runner.cpp)
target_link_libraries(test_script_runner PRIVATE j2534_core)

enable_testing()
add_test(NAME uds_tests COMMAND test_uds)
add_test(NAME isotp_tests COMMAND test_isotp)
add_test(NAME async_uds_tests COMMAND test_async_uds)
add_test(NAME script_runner_tests COMMAND test_script_runner)

option(J2534_BUILD_QT_GUI "Build optional Qt Widgets GUI" OFF)
if (J2534_BUILD_QT_GUI)
    find_package(Qt6 COMPONENTS Widgets REQUIRED)
    qt_standard_project_setup()
    qt_add_executable(j2534_qt_tool
        src/ui/qt_main.cpp
        src/ui/QtMainWindow.cpp
        src/ui/QtMainWindow.h
    )
    target_compile_definitions(j2534_qt_tool PRIVATE J2534_ENABLE_QT)
    target_link_libraries(j2534_qt_tool PRIVATE Qt6::Widgets j2534_core)
    target_include_directories(j2534_qt_tool PRIVATE src)
endif()
