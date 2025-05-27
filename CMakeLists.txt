cmake_minimum_required(VERSION 3.16)
project(HMCLauncher
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_WIN32_EXECUTABLE ON)

add_compile_definitions(UNICODE _UNICODE)
add_compile_definitions(WINVER=0x0601 _WIN32_WINNT=0x0601) # Windows 7

if (MSVC)
    add_compile_options(/utf-8 /W4 /MT)
    add_link_options(/ENTRY:wWinMainCRTStartup)
else ()
    add_compile_options(-municode -Wall -Wextra -Wpedantic)
    add_link_options(-municode -static)
endif ()

add_executable(HMCLauncher WIN32
        HMCL/HMCL.rc HMCL/main.cpp
        HMCL/platform.cpp HMCL/platform.h
        HMCL/i18n.cpp HMCL/i18n.h
        HMCL/java.cpp HMCL/java.h
        HMCL/path.cpp HMCL/path.h
)
target_link_libraries(HMCLauncher PRIVATE Shlwapi Version)
