# Client-only and server-only files are wrapped in EVERSTEAD_DEDICATED_SERVER checks, so both targets take every source
file(GLOB_RECURSE EVERSTEAD_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

set(EVERSTEAD_EXE_TYPE "")

if(WIN32 AND EVERSTEAD_WINDOWS_GUI)
    set(EVERSTEAD_EXE_TYPE WIN32)
endif()

add_executable(Everstead
    ${EVERSTEAD_EXE_TYPE}
    ${EVERSTEAD_SOURCES}
)

if(WIN32)
    target_sources(Everstead PRIVATE icon/Everstead.rc)
endif()

set_target_properties(Everstead PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

string(TOLOWER "${CMAKE_BUILD_TYPE}" EVERSTEAD_BUILD_NAME)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(EVERSTEAD_ARCH_NAME arm64)
else()
    set(EVERSTEAD_ARCH_NAME x86-64)
endif()

if(WIN32)
    set(EVERSTEAD_OS_NAME win)
else()
    set(EVERSTEAD_OS_NAME linux)
endif()

set_target_properties(Everstead PROPERTIES
    OUTPUT_NAME "everstead_${EVERSTEAD_BUILD_NAME}_${EVERSTEAD_ARCH_NAME}_${EVERSTEAD_OS_NAME}"
)

find_program(EVERSTEAD_GLSLANG glslangValidator REQUIRED)
find_program(EVERSTEAD_SPIRV_CROSS spirv-cross REQUIRED)

# Shaders are written once as Vulkan GLSL and compiled to SPIR-V, which every render device
# consumes either directly or through the GLSL and reflection data spirv-cross derives from it.
file(GLOB EVERSTEAD_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
)

file(GLOB EVERSTEAD_SHADER_INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glsl")
set(EVERSTEAD_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin/assets/shader")
set(EVERSTEAD_SHADER_OUTPUTS "")

foreach(EVERSTEAD_SHADER_SOURCE ${EVERSTEAD_SHADER_SOURCES})
    get_filename_component(EVERSTEAD_SHADER_NAME "${EVERSTEAD_SHADER_SOURCE}" NAME)
    set(EVERSTEAD_SHADER_BASE "${EVERSTEAD_SHADER_DIR}/${EVERSTEAD_SHADER_NAME}")
    set(EVERSTEAD_SHADER_GLSL_VERSION 330)
    if(EVERSTEAD_SHADER_NAME MATCHES "\\.comp$")
        set(EVERSTEAD_SHADER_GLSL_VERSION 430)
    endif()

    add_custom_command(
        OUTPUT "${EVERSTEAD_SHADER_BASE}.spv" "${EVERSTEAD_SHADER_BASE}.glsl" "${EVERSTEAD_SHADER_BASE}.json"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${EVERSTEAD_SHADER_DIR}"
        COMMAND "${EVERSTEAD_GLSLANG}" -V --quiet "${EVERSTEAD_SHADER_SOURCE}" -o "${EVERSTEAD_SHADER_BASE}.spv"
        COMMAND "${EVERSTEAD_SPIRV_CROSS}" --version ${EVERSTEAD_SHADER_GLSL_VERSION} --no-es --flatten-ubo --no-420pack-extension
            --output "${EVERSTEAD_SHADER_BASE}.glsl" "${EVERSTEAD_SHADER_BASE}.spv"
        COMMAND "${EVERSTEAD_SPIRV_CROSS}" --reflect --output "${EVERSTEAD_SHADER_BASE}.json" "${EVERSTEAD_SHADER_BASE}.spv"
        DEPENDS "${EVERSTEAD_SHADER_SOURCE}" ${EVERSTEAD_SHADER_INCLUDES}
        VERBATIM
    )
    list(APPEND EVERSTEAD_SHADER_OUTPUTS "${EVERSTEAD_SHADER_BASE}.spv" "${EVERSTEAD_SHADER_BASE}.glsl" "${EVERSTEAD_SHADER_BASE}.json")
endforeach()

add_custom_target(EversteadShaders DEPENDS ${EVERSTEAD_SHADER_OUTPUTS})
add_dependencies(Everstead EversteadShaders)

find_path(EVERSTEAD_VULKAN_INCLUDE_DIR vulkan/vulkan.h REQUIRED)
target_include_directories(Everstead PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/imgui"
    "${EVERSTEAD_VULKAN_INCLUDE_DIR}"
)
target_compile_definitions(Everstead PRIVATE VK_NO_PROTOTYPES GLM_FORCE_CTOR_INIT GLM_ENABLE_EXPERIMENTAL)

if(MSVC)
    target_compile_options(Everstead PRIVATE /W4 /permissive-)
else()
    # Keep diagnostic escape codes when the compiler runs inside a Docker task.
    # VS Code's integrated terminal renders them, even though Docker's stdout
    # is not itself an interactive terminal.
    target_compile_options(Everstead PRIVATE -Wall -Wextra -fdiagnostics-color=always)
endif()

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
# Prefer the package's own CMake config. With a static OpenAL Soft it carries
# transitive dependencies such as fmt, which CMake's bundled FindOpenAL module
# knows nothing about.
find_package(OpenAL CONFIG QUIET)

if(NOT TARGET OpenAL::OpenAL)
    find_package(OpenAL MODULE REQUIRED)
endif()

set(EVERSTEAD_SDL_TARGET "")
set(EVERSTEAD_SDLMAIN_TARGET "")

find_package(SDL2 CONFIG QUIET)

if(TARGET SDL2::SDL2main)
    set(EVERSTEAD_SDLMAIN_TARGET SDL2::SDL2main)
endif()

if(TARGET SDL2::SDL2)
    set(EVERSTEAD_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(EVERSTEAD_SDL_TARGET SDL2::SDL2-static)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    set(EVERSTEAD_SDL_TARGET PkgConfig::SDL2)
endif()

add_library(EversteadMimalloc STATIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mimalloc/src/static.c")
target_include_directories(EversteadMimalloc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mimalloc/include")
target_compile_definitions(EversteadMimalloc PRIVATE MI_SECURE=0 MI_STAT=0)
set_target_properties(EversteadMimalloc PROPERTIES C_STANDARD 11)
if(NOT WIN32)
    # Replaces malloc/free process-wide so memory freed on other threads goes back to the OS.
    target_compile_definitions(EversteadMimalloc PRIVATE MI_MALLOC_OVERRIDE MI_NO_NEW_DELETE_OVERRIDE)
else()
    target_link_libraries(EversteadMimalloc PRIVATE advapi32 bcrypt psapi)
endif()

set(EVERSTEAD_ENET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/enet")
add_library(EversteadEnet STATIC
    "${EVERSTEAD_ENET_DIR}/callbacks.c"
    "${EVERSTEAD_ENET_DIR}/compress.c"
    "${EVERSTEAD_ENET_DIR}/host.c"
    "${EVERSTEAD_ENET_DIR}/list.c"
    "${EVERSTEAD_ENET_DIR}/packet.c"
    "${EVERSTEAD_ENET_DIR}/peer.c"
    "${EVERSTEAD_ENET_DIR}/protocol.c"
    "${EVERSTEAD_ENET_DIR}/unix.c"
    "${EVERSTEAD_ENET_DIR}/win32.c"
)
target_include_directories(EversteadEnet PUBLIC "${EVERSTEAD_ENET_DIR}/include")
set_target_properties(EversteadEnet PROPERTIES C_STANDARD 99)
if(WIN32)
    target_link_libraries(EversteadEnet PUBLIC ws2_32 winmm)
else()
    target_compile_definitions(EversteadEnet PRIVATE
        HAS_FCNTL=1 HAS_POLL=1 HAS_GETADDRINFO=1 HAS_GETNAMEINFO=1 HAS_INET_PTON=1 HAS_INET_NTOP=1
        HAS_MSGHDR_FLAGS=1 HAS_SOCKLEN_T=1)
endif()

add_subdirectory(third_party/luajit)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(GLOB_RECURSE EVERSTEAD_MOD_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ModClasses.cpp" "${CMAKE_SOURCE_DIR}/modkit/library/game.lua"
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/cmake/GenerateModBindings.py" "${CMAKE_CURRENT_SOURCE_DIR}/src"
            "${CMAKE_CURRENT_BINARY_DIR}/ModClasses.cpp" "${CMAKE_SOURCE_DIR}/modkit/library/game.lua"
    DEPENDS ${EVERSTEAD_MOD_SOURCES} "${CMAKE_SOURCE_DIR}/cmake/GenerateModBindings.py"
    VERBATIM
)
target_sources(Everstead PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/ModClasses.cpp")

# Every game function starts with a nop pad that mods overwrite with a jump to their hook
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    target_compile_options(Everstead PRIVATE -fpatchable-function-entry=1)
else()
    target_compile_options(Everstead PRIVATE -fpatchable-function-entry=5)
endif()

target_link_libraries(Everstead PRIVATE
    EversteadMimalloc
    EversteadEnet
    EversteadLuaJit
    ${EVERSTEAD_SDLMAIN_TARGET}
    ${EVERSTEAD_SDL_TARGET}
    OpenAL::OpenAL
    OpenGL::GL
    Freetype::Freetype
    ZLIB::ZLIB
    Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_link_libraries(Everstead PRIVATE ${CMAKE_DL_LIBS})
endif()

if(WIN32)
    target_link_libraries(Everstead PRIVATE ws2_32 iphlpapi psapi dbghelp shell32 ole32)
endif()

if(EVERSTEAD_STATIC_GCC_RUNTIME AND NOT MSVC)
    if(MINGW)
        # -static also pulls in libwinpthread, which -static-libgcc leaves behind.
        target_link_options(Everstead PRIVATE -static)
        # Unoptimized C++23 code emits an inline std::type_info::operator== that static libstdc++ also defines
        target_link_options(Everstead PRIVATE -Wl,--allow-multiple-definition)
    else()
        target_link_options(Everstead PRIVATE -static-libstdc++ -static-libgcc)
    endif()
endif()

if(WIN32 AND NOT EVERSTEAD_STATIC_GCC_RUNTIME)
    # CMake command arguments use semicolons as list separators. Encode the
    # vcpkg prefix list so it reaches the runtime-copy script as one argument.
    set(EVERSTEAD_RUNTIME_SEARCH_PREFIXES "${CMAKE_PREFIX_PATH}")
    string(REPLACE ";" "|" EVERSTEAD_RUNTIME_SEARCH_PREFIXES "${EVERSTEAD_RUNTIME_SEARCH_PREFIXES}")

    add_custom_command(TARGET Everstead POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DTARGET_FILE=$<TARGET_FILE:Everstead>"
            "-DTARGET_DIR=$<TARGET_FILE_DIR:Everstead>"
            "-DMANIFEST_FILE=${CMAKE_CURRENT_BINARY_DIR}/Everstead-runtime-dependencies.txt"
            "-DSEARCH_PREFIXES=${EVERSTEAD_RUNTIME_SEARCH_PREFIXES}"
            -P "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake"
        VERBATIM
    )
endif()

add_executable(EversteadServer ${EVERSTEAD_SOURCES})
set_target_properties(EversteadServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    OUTPUT_NAME "everstead_server_${EVERSTEAD_BUILD_NAME}_${EVERSTEAD_ARCH_NAME}_${EVERSTEAD_OS_NAME}"
)
target_include_directories(EversteadServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(EversteadServer PRIVATE EVERSTEAD_DEDICATED_SERVER GLM_FORCE_CTOR_INIT GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(EversteadServer PRIVATE EversteadMimalloc EversteadEnet ZLIB::ZLIB Threads::Threads)
if(WIN32)
    target_link_libraries(EversteadServer PRIVATE ws2_32 iphlpapi psapi dbghelp shell32 ole32)
else()
    # Exporting symbols lets crash report backtraces show function names
    set_target_properties(EversteadServer PROPERTIES ENABLE_EXPORTS ON)
endif()
