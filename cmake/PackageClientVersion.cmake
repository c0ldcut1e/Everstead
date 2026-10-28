if(NOT DEFINED VERSION OR NOT VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+(-[A-Za-z0-9]+)?$")
    message(FATAL_ERROR "VERSION must use the form 1.2.3 or 1.2.3-name")
endif()

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH PROJECT_ROOT)
set(CLIENT_DIRECTORY "${PROJECT_ROOT}/game/bin")
set(WINDOWS_EXECUTABLE "everstead_release_x86-64_win.exe")
set(LINUX_X64_EXECUTABLE "everstead_release_x86-64_linux")
set(LINUX_ARM64_EXECUTABLE "everstead_release_arm64_linux")
set(SERVER_WINDOWS_EXECUTABLE "everstead_server_release_x86-64_win.exe")
set(SERVER_LINUX_X64_EXECUTABLE "everstead_server_release_x86-64_linux")
set(SERVER_LINUX_ARM64_EXECUTABLE "everstead_server_release_arm64_linux")
set(TARGET_DIRECTORY "${PROJECT_ROOT}/r2/versions/${VERSION}")
set(WORK_DIRECTORY "${PROJECT_ROOT}/build/r2-client-${VERSION}")
set(OUTPUT_DIRECTORY "${WORK_DIRECTORY}/output")
set(PAYLOAD_DIRECTORY "${WORK_DIRECTORY}/payload")
set(SERVER_PAYLOAD_DIRECTORY "${WORK_DIRECTORY}/server-payload")

foreach(REQUIRED_PATH
        "${CLIENT_DIRECTORY}/${WINDOWS_EXECUTABLE}"
        "${CLIENT_DIRECTORY}/${LINUX_X64_EXECUTABLE}"
        "${CLIENT_DIRECTORY}/${LINUX_ARM64_EXECUTABLE}"
        "${CLIENT_DIRECTORY}/${SERVER_WINDOWS_EXECUTABLE}"
        "${CLIENT_DIRECTORY}/${SERVER_LINUX_X64_EXECUTABLE}"
        "${CLIENT_DIRECTORY}/${SERVER_LINUX_ARM64_EXECUTABLE}")
    if(NOT EXISTS "${REQUIRED_PATH}" OR IS_DIRECTORY "${REQUIRED_PATH}")
        message(FATAL_ERROR "Missing release build: ${REQUIRED_PATH}")
    endif()
endforeach()

if(NOT IS_DIRECTORY "${CLIENT_DIRECTORY}/assets")
    message(FATAL_ERROR "Missing game assets: ${CLIENT_DIRECTORY}/assets")
endif()

if(EXISTS "${TARGET_DIRECTORY}")
    message(FATAL_ERROR "Client version ${VERSION} already exists in r2")
endif()

file(REMOVE_RECURSE "${WORK_DIRECTORY}")
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}" "${PAYLOAD_DIRECTORY}" "${SERVER_PAYLOAD_DIRECTORY}")
file(COPY "${CLIENT_DIRECTORY}/assets" DESTINATION "${PAYLOAD_DIRECTORY}")

function(CreateArchive PAYLOAD EXECUTABLE ARCHIVE FORMAT COMPRESSION LEVEL RESULT_PREFIX)
    file(COPY "${CLIENT_DIRECTORY}/${EXECUTABLE}" DESTINATION "${PAYLOAD}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DARCHIVE_OUTPUT=${OUTPUT_DIRECTORY}/${ARCHIVE}"
            "-DARCHIVE_FORMAT=${FORMAT}"
            "-DARCHIVE_COMPRESSION=${COMPRESSION}"
            "-DARCHIVE_LEVEL=${LEVEL}"
            -P "${CMAKE_CURRENT_LIST_DIR}/CreateReleaseArchive.cmake"
        WORKING_DIRECTORY "${PAYLOAD}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    file(REMOVE "${PAYLOAD}/${EXECUTABLE}")
    file(SHA256 "${OUTPUT_DIRECTORY}/${ARCHIVE}" ARCHIVE_HASH)
    file(SIZE "${OUTPUT_DIRECTORY}/${ARCHIVE}" ARCHIVE_SIZE)
    set("${RESULT_PREFIX}_HASH" "${ARCHIVE_HASH}" PARENT_SCOPE)
    set("${RESULT_PREFIX}_SIZE" "${ARCHIVE_SIZE}" PARENT_SCOPE)
endfunction()

set(WINDOWS_ARCHIVE "everstead_release_x86-64_win.zip")
set(LINUX_X64_ARCHIVE "everstead_release_x86-64_linux.tar.zst")
set(LINUX_ARM64_ARCHIVE "everstead_release_arm64_linux.tar.zst")
set(SERVER_WINDOWS_ARCHIVE "everstead_server_release_x86-64_win.zip")
set(SERVER_LINUX_X64_ARCHIVE "everstead_server_release_x86-64_linux.tar.zst")
set(SERVER_LINUX_ARM64_ARCHIVE "everstead_server_release_arm64_linux.tar.zst")
CreateArchive("${PAYLOAD_DIRECTORY}" "${WINDOWS_EXECUTABLE}" "${WINDOWS_ARCHIVE}" zip Deflate 9 WINDOWS)
CreateArchive("${PAYLOAD_DIRECTORY}" "${LINUX_X64_EXECUTABLE}" "${LINUX_X64_ARCHIVE}" gnutar Zstd 19 LINUX_X64)
CreateArchive("${PAYLOAD_DIRECTORY}" "${LINUX_ARM64_EXECUTABLE}" "${LINUX_ARM64_ARCHIVE}" gnutar Zstd 19 LINUX_ARM64)
CreateArchive("${SERVER_PAYLOAD_DIRECTORY}" "${SERVER_WINDOWS_EXECUTABLE}" "${SERVER_WINDOWS_ARCHIVE}" zip Deflate 9 SERVER_WINDOWS)
CreateArchive("${SERVER_PAYLOAD_DIRECTORY}" "${SERVER_LINUX_X64_EXECUTABLE}" "${SERVER_LINUX_X64_ARCHIVE}" gnutar Zstd 19 SERVER_LINUX_X64)
CreateArchive("${SERVER_PAYLOAD_DIRECTORY}" "${SERVER_LINUX_ARM64_EXECUTABLE}" "${SERVER_LINUX_ARM64_ARCHIVE}" gnutar Zstd 19 SERVER_LINUX_ARM64)

set(MANIFEST "${OUTPUT_DIRECTORY}/manifest.json")
function(AppendRelease PLATFORM PREFIX SEPARATOR)
    file(APPEND "${MANIFEST}" "        \"${PLATFORM}\": {\n")
    file(APPEND "${MANIFEST}" "            \"file\": \"${${PREFIX}_ARCHIVE}\",\n")
    file(APPEND "${MANIFEST}" "            \"executable\": \"${${PREFIX}_EXECUTABLE}\",\n")
    file(APPEND "${MANIFEST}" "            \"sha256\": \"${${PREFIX}_HASH}\",\n")
    file(APPEND "${MANIFEST}" "            \"size\": ${${PREFIX}_SIZE}\n")
    file(APPEND "${MANIFEST}" "        }${SEPARATOR}\n")
endfunction()

file(WRITE "${MANIFEST}" "{\n")
file(APPEND "${MANIFEST}" "    \"version\": \"${VERSION}\",\n")
file(APPEND "${MANIFEST}" "    \"releases\": {\n")
AppendRelease(windows-x64 WINDOWS ",")
AppendRelease(linux-x64 LINUX_X64 ",")
AppendRelease(linux-arm64 LINUX_ARM64 "")
file(APPEND "${MANIFEST}" "    },\n")
file(APPEND "${MANIFEST}" "    \"servers\": {\n")
AppendRelease(windows-x64 SERVER_WINDOWS ",")
AppendRelease(linux-x64 SERVER_LINUX_X64 ",")
AppendRelease(linux-arm64 SERVER_LINUX_ARM64 "")
file(APPEND "${MANIFEST}" "    }\n")
file(APPEND "${MANIFEST}" "}\n")

file(MAKE_DIRECTORY "${PROJECT_ROOT}/r2/versions")
if(EXISTS "${TARGET_DIRECTORY}")
    message(FATAL_ERROR "Client version ${VERSION} already exists in r2")
endif()

file(RENAME "${OUTPUT_DIRECTORY}" "${TARGET_DIRECTORY}" RESULT RENAME_RESULT)
if(RENAME_RESULT)
    message(FATAL_ERROR "Could not add client version to r2: ${RENAME_RESULT}")
endif()

file(REMOVE_RECURSE "${WORK_DIRECTORY}")
message(STATUS "Added client version ${VERSION} to ${TARGET_DIRECTORY}")
