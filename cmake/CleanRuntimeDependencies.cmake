if(NOT DEFINED TARGET_DIR)
    message(FATAL_ERROR "TARGET_DIR is required")
endif()

set(manifestsToClean)
if(DEFINED MANIFEST_FILE)
    if(EXISTS "${MANIFEST_FILE}")
        list(APPEND manifestsToClean "${MANIFEST_FILE}")
    endif()
elseif(DEFINED BUILD_ROOT)
    file(GLOB manifestsToClean "${BUILD_ROOT}/docker-windows-*/Everstead-runtime-dependencies.txt")
else()
    message(FATAL_ERROR "MANIFEST_FILE or BUILD_ROOT is required")
endif()

# Preserve a DLL used by another Windows configuration that has not been
# cleaned yet. This matters because Debug and Release share bin/.
set(allManifests)
if(DEFINED BUILD_ROOT)
    file(GLOB allManifests "${BUILD_ROOT}/docker-windows-*/Everstead-runtime-dependencies.txt")
else()
    set(allManifests "${manifestsToClean}")
endif()

set(runtimeFilesToRemove)
set(runtimeFilesToKeep)
foreach(manifest IN LISTS allManifests)
    file(STRINGS "${manifest}" manifestRuntimeFiles)
    list(FIND manifestsToClean "${manifest}" manifestIndex)
    if(manifestIndex EQUAL -1)
        list(APPEND runtimeFilesToKeep ${manifestRuntimeFiles})
    else()
        list(APPEND runtimeFilesToRemove ${manifestRuntimeFiles})
    endif()
endforeach()

# Clean All Builds also removes leftovers from builds created before manifests
# existed. This is extension-based discovery, not a hardcoded dependency list.
if(REMOVE_ALL_RUNTIME_DLLS)
    file(GLOB runtimeDlls "${TARGET_DIR}/*.dll" "${TARGET_DIR}/*.DLL")
    list(APPEND runtimeFilesToRemove ${runtimeDlls})
endif()

list(REMOVE_DUPLICATES runtimeFilesToRemove)
list(REMOVE_DUPLICATES runtimeFilesToKeep)
file(REAL_PATH "${TARGET_DIR}" resolvedTargetDirectory)

foreach(runtimeFile IN LISTS runtimeFilesToRemove)
    cmake_path(NORMAL_PATH runtimeFile OUTPUT_VARIABLE normalizedRuntimeFile)
    cmake_path(IS_PREFIX resolvedTargetDirectory "${normalizedRuntimeFile}" NORMALIZE isInTargetDirectory)
    if(NOT isInTargetDirectory)
        message(FATAL_ERROR "Refusing to remove a runtime file outside TARGET_DIR: ${runtimeFile}")
    endif()

    list(FIND runtimeFilesToKeep "${runtimeFile}" retainedIndex)
    if(retainedIndex EQUAL -1)
        file(REMOVE "${normalizedRuntimeFile}")
    endif()
endforeach()

foreach(manifest IN LISTS manifestsToClean)
    file(REMOVE "${manifest}")
endforeach()
