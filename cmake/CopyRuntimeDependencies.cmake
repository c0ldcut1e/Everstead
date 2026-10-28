if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

if(NOT DEFINED TARGET_FILE)
    message(FATAL_ERROR "TARGET_FILE is required")
endif()

if(NOT DEFINED TARGET_DIR)
    message(FATAL_ERROR "TARGET_DIR is required")
endif()

if(NOT DEFINED MANIFEST_FILE)
    message(FATAL_ERROR "MANIFEST_FILE is required")
endif()

# The build command encodes CMake's list separator so the value survives as a
# single command-line argument.
string(REPLACE "|" ";" SEARCH_PREFIXES "${SEARCH_PREFIXES}")

set(searchDirectories)
foreach(prefix IN LISTS SEARCH_PREFIXES)
    if(IS_DIRECTORY "${prefix}/bin")
        list(APPEND searchDirectories "${prefix}/bin")
    endif()

    if(IS_DIRECTORY "${prefix}")
        list(APPEND searchDirectories "${prefix}")
    endif()
endforeach()

# CMake cannot scan MinGW PE executables with GET_RUNTIME_DEPENDENCIES on the
# Linux host. Copy DLLs from each configured package/runtime prefix instead.
set(runtimeDependencies)
foreach(searchDirectory IN LISTS searchDirectories)
    file(GLOB directoryDependencies
        "${searchDirectory}/*.dll"
        "${searchDirectory}/*.DLL"
    )
    list(APPEND runtimeDependencies ${directoryDependencies})
endforeach()
list(REMOVE_DUPLICATES runtimeDependencies)

set(runtimeOutputs)
foreach(dependency IN LISTS runtimeDependencies)
    get_filename_component(dependencyName "${dependency}" NAME)
    set(runtimeOutput "${TARGET_DIR}/${dependencyName}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dependency}" "${runtimeOutput}"
        RESULT_VARIABLE copyResult
    )

    if(NOT copyResult EQUAL 0)
        message(FATAL_ERROR "Failed to copy runtime dependency: ${dependency}")
    endif()

    list(APPEND runtimeOutputs "${runtimeOutput}")
endforeach()

# Record the copied paths per build directory. Clean tasks consume this file
# instead of having a brittle list of DLL filenames.
list(REMOVE_DUPLICATES runtimeOutputs)
file(WRITE "${MANIFEST_FILE}" "")
foreach(runtimeOutput IN LISTS runtimeOutputs)
    file(APPEND "${MANIFEST_FILE}" "${runtimeOutput}\n")
endforeach()
