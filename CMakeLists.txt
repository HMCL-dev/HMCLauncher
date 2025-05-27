cmake_minimum_required(VERSION 3.16)
project(HMCLauncher
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_WIN32_EXECUTABLE ON)

add_compile_definitions(-DUNICODE -D_UNICODE)
add_compile_definitions(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601) # Windows 7

if (MSVC)
    add_compile_options(/utf-8 /W4 /MT)
    add_link_options(/ENTRY:wWinMainCRTStartup)
else ()
    add_compile_options(-municode -Wall -Wextra -Wpedantic)
    add_link_options(-municode -static)
endif ()

add_executable(HMCLauncher WIN32 HMCL/HMCL.rc
        HMCL/main.cpp
        HMCL/os.cpp HMCL/os.h
        HMCL/i18n.cpp
        HMCL/i18n.h
        HMCL/ui.cpp
        HMCL/ui.h
        HMCL/java.cpp
        HMCL/java.h
)
target_link_libraries(HMCLauncher Version)
