find_package(LibArchive REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml Quick)

set(EVERSTEAD_LAUNCHER_VERSION "0.2.0")

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(EversteadLauncher
    src/GameLogModel.cpp
    src/GameLogModel.h
    src/LauncherState.cpp
    src/LauncherState.h
    src/LauncherUpdater.cpp
    src/LauncherUpdater.h
    src/Main.cpp
)

qt_add_qml_module(EversteadLauncher
    URI Everstead.Launcher
    VERSION 1.0
    NO_IMPORT_SCAN
    QML_FILES
        qml/Main.qml
)

set_target_properties(EversteadLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

string(TOLOWER "${CMAKE_BUILD_TYPE}" EVERSTEAD_LAUNCHER_BUILD_NAME)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(EVERSTEAD_LAUNCHER_ARCH_NAME arm64)
else()
    set(EVERSTEAD_LAUNCHER_ARCH_NAME x86-64)
endif()

if(WIN32)
    set(EVERSTEAD_LAUNCHER_OS_NAME win)
else()
    set(EVERSTEAD_LAUNCHER_OS_NAME linux)
endif()

set_target_properties(EversteadLauncher PROPERTIES
    OUTPUT_NAME "everstead_launcher_${EVERSTEAD_LAUNCHER_BUILD_NAME}_${EVERSTEAD_LAUNCHER_ARCH_NAME}_${EVERSTEAD_LAUNCHER_OS_NAME}"
)

if(WIN32)
    set(EVERSTEAD_LAUNCHER_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/everstead_launcher_${EVERSTEAD_LAUNCHER_BUILD_NAME}_${EVERSTEAD_LAUNCHER_ARCH_NAME}_${EVERSTEAD_LAUNCHER_OS_NAME}"
    )
    set_target_properties(EversteadLauncher PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${EVERSTEAD_LAUNCHER_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${EVERSTEAD_LAUNCHER_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${EVERSTEAD_LAUNCHER_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${EVERSTEAD_LAUNCHER_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${EVERSTEAD_LAUNCHER_OUTPUT_DIRECTORY}"
    )
endif()

qt_add_resources(EversteadLauncher "icon"
    PREFIX "/"
    FILES icon/Launcher.png
)

if(WIN32)
    set_target_properties(EversteadLauncher PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(EversteadLauncher PRIVATE icon/Launcher.rc)
endif()

target_link_libraries(EversteadLauncher PRIVATE
    LibArchive::LibArchive
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(EVERSTEAD_ZSTD_STATIC_LIBRARY NAMES libzstd.a REQUIRED)
    target_link_libraries(EversteadLauncher PRIVATE util ${EVERSTEAD_ZSTD_STATIC_LIBRARY})
endif()

target_compile_definitions(EversteadLauncher PRIVATE
    EVERSTEAD_LAUNCHER_VERSION="${EVERSTEAD_LAUNCHER_VERSION}"
)

if(WIN32)
    target_link_libraries(EversteadLauncher PRIVATE bcrypt)

    add_custom_command(TARGET EversteadLauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:EversteadLauncher>
            $<TARGET_FILE_DIR:EversteadLauncher>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/libgcc_s_seh-1.dll
            $<TARGET_FILE_DIR:Qt6::Core>/libstdc++-6.dll
            $<TARGET_FILE_DIR:Qt6::Core>/libwinpthread-1.dll
            $<TARGET_FILE_DIR:EversteadLauncher>
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:EversteadLauncher>/platforms
            $<TARGET_FILE_DIR:EversteadLauncher>/tls
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQml/Models
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQml/WorkerScript
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQuick/Window
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../plugins/platforms/qwindows.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../plugins/tls/qschannelbackend.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/tls
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/qmldir
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/plugins.qmltypes
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/qmlplugin.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQml
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/Models/qmldir
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/Models/plugins.qmltypes
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/Models/modelsplugin.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQml/Models
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/WorkerScript/qmldir
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/WorkerScript/plugins.qmltypes
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQml/WorkerScript/workerscriptplugin.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQml/WorkerScript
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQuick/qmldir
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQuick/plugins.qmltypes
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQuick/qtquick2plugin.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQuick
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQuick/Window/qmldir
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQuick/Window/quickwindow.qmltypes
            $<TARGET_FILE_DIR:Qt6::Core>/../qml/QtQuick/Window/quickwindowplugin.dll
            $<TARGET_FILE_DIR:EversteadLauncher>/qml/QtQuick/Window
        COMMAND_EXPAND_LISTS
    )
endif()
