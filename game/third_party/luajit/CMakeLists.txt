set(LUAJIT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LUAJIT_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${LUAJIT_GENERATED_DIR}")

# The VM builder has to run on the build machine, so it is compiled with the host compiler even when cross compiling
find_program(LUAJIT_HOST_CC NAMES cc clang gcc REQUIRED NO_CMAKE_FIND_ROOT_PATH)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(LUAJIT_ARCH arm64)
else()
    set(LUAJIT_ARCH x64)
endif()

execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -E -dM "${LUAJIT_SOURCE_DIR}/lj_arch.h"
    OUTPUT_VARIABLE LUAJIT_TESTARCH
    COMMAND_ERROR_IS_FATAL ANY
)

set(LUAJIT_DASM_FLAGS "")
set(LUAJIT_DASM_ARCH ${LUAJIT_ARCH})
foreach(LUAJIT_DASM_PAIR
        "LJ_LE 1:ENDIAN_LE" "LJ_ARCH_BITS 64:P64" "LJ_HASJIT 1:JIT" "LJ_HASFFI 1:FFI" "LJ_DUALNUM 1:DUALNUM"
        "LJ_ARCH_HASFPU 1:FPU" "LJ_NO_UNWIND 1:NO_UNWIND" "LJ_ABI_PAUTH 1:PAUTH" "LJ_ABI_BRANCH_TRACK 1:BRANCH_TRACK"
        "LJ_ABI_SHADOW_STACK 1:SHADOW_STACK")
    string(REPLACE ":" ";" LUAJIT_DASM_PAIR "${LUAJIT_DASM_PAIR}")
    list(GET LUAJIT_DASM_PAIR 0 LUAJIT_DASM_DEFINE)
    list(GET LUAJIT_DASM_PAIR 1 LUAJIT_DASM_FLAG)
    if(LUAJIT_TESTARCH MATCHES "#define ${LUAJIT_DASM_DEFINE}\n")
        list(APPEND LUAJIT_DASM_FLAGS -D ${LUAJIT_DASM_FLAG})
    endif()
endforeach()

if(NOT LUAJIT_TESTARCH MATCHES "#define LJ_ABI_SOFTFP 1\n")
    list(APPEND LUAJIT_DASM_FLAGS -D HFABI)
endif()

string(REGEX MATCH "#define LJ_ARCH_VERSION ([0-9]+)" LUAJIT_ARCH_VERSION_MATCH "${LUAJIT_TESTARCH}")
list(APPEND LUAJIT_DASM_FLAGS -D VER=${CMAKE_MATCH_1})

if(LUAJIT_ARCH STREQUAL "x64" AND NOT LUAJIT_TESTARCH MATCHES "#define LJ_FR2 1\n")
    set(LUAJIT_DASM_ARCH x86)
endif()

set(LUAJIT_HOST_FLAGS -DLUAJIT_TARGET=LUAJIT_ARCH_${LUAJIT_ARCH})
if(WIN32)
    list(APPEND LUAJIT_DASM_FLAGS -D WIN)
    list(APPEND LUAJIT_HOST_FLAGS -malign-double -DLUAJIT_OS=LUAJIT_OS_WINDOWS)
    set(LUAJIT_VM_MODE peobj)
    set(LUAJIT_VM "${LUAJIT_GENERATED_DIR}/lj_vm.o")
    set_source_files_properties("${LUAJIT_VM}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
else()
    list(APPEND LUAJIT_HOST_FLAGS -DLUAJIT_OS=LUAJIT_OS_LINUX)
    set(LUAJIT_VM_MODE elfasm)
    set(LUAJIT_VM "${LUAJIT_GENERATED_DIR}/lj_vm.S")
    enable_language(ASM)
endif()

set(LUAJIT_MINILUA "${CMAKE_CURRENT_BINARY_DIR}/minilua")
add_custom_command(
    OUTPUT "${LUAJIT_MINILUA}"
    COMMAND "${LUAJIT_HOST_CC}" -O2 -o "${LUAJIT_MINILUA}" "${LUAJIT_SOURCE_DIR}/host/minilua.c" -lm
    DEPENDS "${LUAJIT_SOURCE_DIR}/host/minilua.c"
    VERBATIM
)

add_custom_command(
    OUTPUT "${LUAJIT_GENERATED_DIR}/buildvm_arch.h"
    COMMAND "${LUAJIT_MINILUA}" "${CMAKE_CURRENT_SOURCE_DIR}/dynasm/dynasm.lua" ${LUAJIT_DASM_FLAGS}
        -o "${LUAJIT_GENERATED_DIR}/buildvm_arch.h" "${LUAJIT_SOURCE_DIR}/vm_${LUAJIT_DASM_ARCH}.dasc"
    DEPENDS "${LUAJIT_MINILUA}" "${LUAJIT_SOURCE_DIR}/vm_${LUAJIT_DASM_ARCH}.dasc"
    VERBATIM
)

set(LUAJIT_BUILDVM "${CMAKE_CURRENT_BINARY_DIR}/buildvm")
set(LUAJIT_BUILDVM_SOURCES
    "${LUAJIT_SOURCE_DIR}/host/buildvm.c"
    "${LUAJIT_SOURCE_DIR}/host/buildvm_asm.c"
    "${LUAJIT_SOURCE_DIR}/host/buildvm_peobj.c"
    "${LUAJIT_SOURCE_DIR}/host/buildvm_lib.c"
    "${LUAJIT_SOURCE_DIR}/host/buildvm_fold.c"
)
add_custom_command(
    OUTPUT "${LUAJIT_BUILDVM}"
    COMMAND "${LUAJIT_HOST_CC}" -O2 ${LUAJIT_HOST_FLAGS} -I "${LUAJIT_GENERATED_DIR}" -I "${LUAJIT_SOURCE_DIR}"
        -o "${LUAJIT_BUILDVM}" ${LUAJIT_BUILDVM_SOURCES}
    DEPENDS ${LUAJIT_BUILDVM_SOURCES} "${LUAJIT_GENERATED_DIR}/buildvm_arch.h"
    VERBATIM
)

set(LUAJIT_LIB_SOURCES
    "${LUAJIT_SOURCE_DIR}/lib_base.c"
    "${LUAJIT_SOURCE_DIR}/lib_math.c"
    "${LUAJIT_SOURCE_DIR}/lib_bit.c"
    "${LUAJIT_SOURCE_DIR}/lib_string.c"
    "${LUAJIT_SOURCE_DIR}/lib_table.c"
    "${LUAJIT_SOURCE_DIR}/lib_io.c"
    "${LUAJIT_SOURCE_DIR}/lib_os.c"
    "${LUAJIT_SOURCE_DIR}/lib_package.c"
    "${LUAJIT_SOURCE_DIR}/lib_debug.c"
    "${LUAJIT_SOURCE_DIR}/lib_jit.c"
    "${LUAJIT_SOURCE_DIR}/lib_ffi.c"
    "${LUAJIT_SOURCE_DIR}/lib_buffer.c"
)

add_custom_command(
    OUTPUT "${LUAJIT_VM}"
    COMMAND "${LUAJIT_BUILDVM}" -m ${LUAJIT_VM_MODE} -o "${LUAJIT_VM}"
    DEPENDS "${LUAJIT_BUILDVM}"
    VERBATIM
)

set(LUAJIT_HEADERS "")
foreach(LUAJIT_HEADER_MODE bcdef ffdef libdef recdef)
    add_custom_command(
        OUTPUT "${LUAJIT_GENERATED_DIR}/lj_${LUAJIT_HEADER_MODE}.h"
        COMMAND "${LUAJIT_BUILDVM}" -m ${LUAJIT_HEADER_MODE} -o "${LUAJIT_GENERATED_DIR}/lj_${LUAJIT_HEADER_MODE}.h" ${LUAJIT_LIB_SOURCES}
        DEPENDS "${LUAJIT_BUILDVM}" ${LUAJIT_LIB_SOURCES}
        VERBATIM
    )
    list(APPEND LUAJIT_HEADERS "${LUAJIT_GENERATED_DIR}/lj_${LUAJIT_HEADER_MODE}.h")
endforeach()

add_custom_command(
    OUTPUT "${LUAJIT_GENERATED_DIR}/lj_folddef.h"
    COMMAND "${LUAJIT_BUILDVM}" -m folddef -o "${LUAJIT_GENERATED_DIR}/lj_folddef.h" "${LUAJIT_SOURCE_DIR}/lj_opt_fold.c"
    DEPENDS "${LUAJIT_BUILDVM}" "${LUAJIT_SOURCE_DIR}/lj_opt_fold.c"
    VERBATIM
)
list(APPEND LUAJIT_HEADERS "${LUAJIT_GENERATED_DIR}/lj_folddef.h")

file(GLOB LUAJIT_CORE_SOURCES "${LUAJIT_SOURCE_DIR}/lj_*.c")
add_library(EversteadLuaJit STATIC
    ${LUAJIT_CORE_SOURCES}
    ${LUAJIT_LIB_SOURCES}
    "${LUAJIT_SOURCE_DIR}/lib_aux.c"
    "${LUAJIT_SOURCE_DIR}/lib_init.c"
    ${LUAJIT_HEADERS}
    "${LUAJIT_VM}"
)
target_include_directories(EversteadLuaJit PRIVATE "${LUAJIT_GENERATED_DIR}")
target_include_directories(EversteadLuaJit PUBLIC "${LUAJIT_SOURCE_DIR}")
target_compile_definitions(EversteadLuaJit PRIVATE _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE)
target_compile_options(EversteadLuaJit PRIVATE -U_FORTIFY_SOURCE -fno-stack-protector -w)
set_target_properties(EversteadLuaJit PROPERTIES C_STANDARD 99)
if(NOT WIN32)
    target_compile_definitions(EversteadLuaJit PRIVATE LUAJIT_UNWIND_EXTERNAL)
    target_link_libraries(EversteadLuaJit PUBLIC m ${CMAKE_DL_LIBS})
endif()
