cmake_minimum_required(VERSION 3.21)

project(Everstead
    VERSION 0.1.0
    LANGUAGES C CXX
)

option(EVERSTEAD_STATIC_GCC_RUNTIME "Link libstdc++/libgcc statically when using GCC/MinGW" ON)
option(EVERSTEAD_WINDOWS_GUI "Build without a console window on Windows" OFF)
option(EVERSTEAD_BUILD_CLIENT "Build the Everstead game client" ON)
option(EVERSTEAD_BUILD_LAUNCHER "Build the Everstead launcher" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EVERSTEAD_BUILD_CLIENT)
    add_subdirectory(game)
endif()

if(EVERSTEAD_BUILD_LAUNCHER)
    add_subdirectory(launcher)
endif()
