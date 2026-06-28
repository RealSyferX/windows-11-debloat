cmake_minimum_required(VERSION 3.15)
project(Windows11Debloat CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS UNICODE _UNICODE NOMINMAX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /utf-8")
endif()

add_executable(Debloat
    src/main.cpp
    src/Utils.cpp
    src/AppxManager.cpp
    src/ServiceManager.cpp
    src/TelemetryManager.cpp
    src/ScheduledTaskManager.cpp
    src/HostsManager.cpp
    src/PerformanceManager.cpp
)

target_include_directories(Debloat PRIVATE src)

target_link_libraries(Debloat PRIVATE advapi32 shell32)

# Auto-prompt UAC (requireAdministrator) via linker-generated manifest
if(MSVC)
    set_target_properties(Debloat PROPERTIES
        LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\""
    )
endif()

# ---- Unit tests -------------------------------------------------------------
# Enable with: cmake -B build -G "Visual Studio 17 2022" -DBUILD_TESTING=ON
# The test target does NOT include main.cpp (it provides its own main()).
# Utils.cpp is included because every manager .cpp pulls in Utils.h; linking
# it keeps the test working in both Debug and Release configurations.
option(BUILD_TESTING "Build unit tests" OFF)
if(BUILD_TESTING)
    add_executable(debloat_tests
        tests/test_datatables.cpp
        src/AppxManager.cpp
        src/ServiceManager.cpp
        src/ScheduledTaskManager.cpp
        src/HostsManager.cpp
        src/TelemetryManager.cpp
        src/Utils.cpp
    )
    target_include_directories(debloat_tests PRIVATE src)
    target_compile_features(debloat_tests PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(debloat_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(debloat_tests PRIVATE advapi32 shell32)
endif()
