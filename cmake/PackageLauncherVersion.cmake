if(NOT DEFINED VERSION OR NOT VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must use the form 1.2.3")
endif()

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH PROJECT_ROOT)
set(LAUNCHER_DIRECTORY "${PROJECT_ROOT}/launcher/bin")
set(WINDOWS_EXECUTABLE "everstead_launcher_release_x86-64_win.exe")
set(LINUX_X64_EXECUTABLE "everstead_launcher_release_x86-64_linux")
set(LINUX_ARM64_EXECUTABLE "everstead_launcher_release_arm64_linux")
set(WINDOWS_DIRECTORY "${LAUNCHER_DIRECTORY}/everstead_launcher_release_x86-64_win")
set(TARGET_DIRECTORY "${PROJECT_ROOT}/r2/launcher/versions/${VERSION}")
set(WORK_DIRECTORY "${PROJECT_ROOT}/build/r2-launcher-${VERSION}")
set(OUTPUT_DIRECTORY "${WORK_DIRECTORY}/output")

file(STRINGS "${PROJECT_ROOT}/launcher/CMakeLists.txt" LAUNCHER_VERSION_LINE
    REGEX "^set\\(EVERSTEAD_LAUNCHER_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\"\\)$")
if(NOT LAUNCHER_VERSION_LINE STREQUAL "set(EVERSTEAD_LAUNCHER_VERSION \"${VERSION}\")")
    message(FATAL_ERROR "Version ${VERSION} does not match the launcher build version")
endif()

foreach(REQUIRED_PATH
        "${WINDOWS_DIRECTORY}/${WINDOWS_EXECUTABLE}"
        "${LAUNCHER_DIRECTORY}/${LINUX_X64_EXECUTABLE}"
        "${LAUNCHER_DIRECTORY}/${LINUX_ARM64_EXECUTABLE}")
    if(NOT EXISTS "${REQUIRED_PATH}" OR IS_DIRECTORY "${REQUIRED_PATH}")
        message(FATAL_ERROR "Missing release build: ${REQUIRED_PATH}")
    endif()
endforeach()

if(EXISTS "${TARGET_DIRECTORY}")
    message(FATAL_ERROR "Launcher version ${VERSION} already exists in r2")
endif()

file(REMOVE_RECURSE "${WORK_DIRECTORY}")
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")

set(WINDOWS_ARCHIVE "everstead_launcher_release_x86-64_win.zip")
set(LINUX_X64_ARCHIVE "everstead_launcher_release_x86-64_linux.tar.zst")
set(LINUX_ARM64_ARCHIVE "everstead_launcher_release_arm64_linux.tar.zst")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DARCHIVE_OUTPUT=${OUTPUT_DIRECTORY}/${WINDOWS_ARCHIVE}"
        -DARCHIVE_FORMAT=zip
        -DARCHIVE_COMPRESSION=Deflate
        -DARCHIVE_LEVEL=9
        -P "${CMAKE_CURRENT_LIST_DIR}/CreateReleaseArchive.cmake"
    WORKING_DIRECTORY "${WINDOWS_DIRECTORY}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DARCHIVE_OUTPUT=${OUTPUT_DIRECTORY}/${LINUX_X64_ARCHIVE}"
        "-DARCHIVE_PATHS=${LINUX_X64_EXECUTABLE}"
        -DARCHIVE_FORMAT=gnutar
        -DARCHIVE_COMPRESSION=Zstd
        -DARCHIVE_LEVEL=19
        -P "${CMAKE_CURRENT_LIST_DIR}/CreateReleaseArchive.cmake"
    WORKING_DIRECTORY "${LAUNCHER_DIRECTORY}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DARCHIVE_OUTPUT=${OUTPUT_DIRECTORY}/${LINUX_ARM64_ARCHIVE}"
        "-DARCHIVE_PATHS=${LINUX_ARM64_EXECUTABLE}"
        -DARCHIVE_FORMAT=gnutar
        -DARCHIVE_COMPRESSION=Zstd
        -DARCHIVE_LEVEL=19
        -P "${CMAKE_CURRENT_LIST_DIR}/CreateReleaseArchive.cmake"
    WORKING_DIRECTORY "${LAUNCHER_DIRECTORY}"
    COMMAND_ERROR_IS_FATAL ANY
)

file(SHA256 "${OUTPUT_DIRECTORY}/${WINDOWS_ARCHIVE}" WINDOWS_HASH)
file(SIZE "${OUTPUT_DIRECTORY}/${WINDOWS_ARCHIVE}" WINDOWS_SIZE)
file(SHA256 "${OUTPUT_DIRECTORY}/${LINUX_X64_ARCHIVE}" LINUX_X64_HASH)
file(SIZE "${OUTPUT_DIRECTORY}/${LINUX_X64_ARCHIVE}" LINUX_X64_SIZE)
file(SHA256 "${OUTPUT_DIRECTORY}/${LINUX_ARM64_ARCHIVE}" LINUX_ARM64_HASH)
file(SIZE "${OUTPUT_DIRECTORY}/${LINUX_ARM64_ARCHIVE}" LINUX_ARM64_SIZE)

file(WRITE "${OUTPUT_DIRECTORY}/manifest.json" "{\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "    \"version\": \"${VERSION}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "    \"releases\": {\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "        \"windows-x64\": {\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"file\": \"${WINDOWS_ARCHIVE}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"sha256\": \"${WINDOWS_HASH}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"size\": ${WINDOWS_SIZE}\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "        },\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "        \"linux-x64\": {\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"file\": \"${LINUX_X64_ARCHIVE}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"sha256\": \"${LINUX_X64_HASH}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"size\": ${LINUX_X64_SIZE}\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "        },\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "        \"linux-arm64\": {\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"file\": \"${LINUX_ARM64_ARCHIVE}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"sha256\": \"${LINUX_ARM64_HASH}\",\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "            \"size\": ${LINUX_ARM64_SIZE}\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "        }\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "    }\n")
file(APPEND "${OUTPUT_DIRECTORY}/manifest.json" "}\n")

file(MAKE_DIRECTORY "${PROJECT_ROOT}/r2/launcher/versions")
if(EXISTS "${TARGET_DIRECTORY}")
    message(FATAL_ERROR "Launcher version ${VERSION} already exists in r2")
endif()

file(RENAME "${OUTPUT_DIRECTORY}" "${TARGET_DIRECTORY}" RESULT RENAME_RESULT)
if(RENAME_RESULT)
    message(FATAL_ERROR "Could not add launcher version to r2: ${RENAME_RESULT}")
endif()

file(REMOVE_RECURSE "${WORK_DIRECTORY}")
message(STATUS "Added launcher version ${VERSION} to ${TARGET_DIRECTORY}")
