# Used by the Docker-only Windows CMake presets through vcpkg's chainload
# mechanism. MinGW-w64 is the practical option for producing Windows binaries
# from a Linux container; MSVC requires a Windows environment and SDK.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Search target libraries and headers, but continue to run build tools from the
# Linux container.
list(PREPEND CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
list(APPEND CMAKE_PREFIX_PATH /usr/x86_64-w64-mingw32/lib)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
