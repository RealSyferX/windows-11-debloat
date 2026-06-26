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
