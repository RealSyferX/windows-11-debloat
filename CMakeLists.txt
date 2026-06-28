cmake_minimum_required(VERSION 3.15)
project(Windows11Debloat CXX)

set(DEBLOAT_VERSION "1.0.0" CACHE STRING "Debloat version")

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

target_compile_definitions(Debloat PRIVATE DEBLOAT_VERSION="${DEBLOAT_VERSION}")

target_link_libraries(Debloat PRIVATE advapi32 shell32)

# Embed the full application manifest (UAC requireAdministrator, supportedOS
# GUIDs for Windows 10/11, and Common-Controls v6 dependency) from app.manifest.
# /MANIFESTUAC is kept with the same value as app.manifest's
# requestedExecutionLevel so the linker-generated default (asInvoker) does not
# conflict with our requireAdministrator during the manifest merge step.
# The /MANIFESTINPUT path is quoted because CMAKE_SOURCE_DIR contains spaces.
if(MSVC)
    set_target_properties(Debloat PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" \"/MANIFESTINPUT:${CMAKE_SOURCE_DIR}/app.manifest\""
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
    target_compile_definitions(debloat_tests PRIVATE DEBLOAT_VERSION="${DEBLOAT_VERSION}")
    if(MSVC)
        target_compile_options(debloat_tests PRIVATE /utf-8)
    endif()
    target_link_libraries(debloat_tests PRIVATE advapi32 shell32)
endif()
