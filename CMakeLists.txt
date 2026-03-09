cmake_minimum_required(VERSION 3.16)

project(darvin VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        terminalwidget.cpp
        terminalwidget.h
        resources.rc
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(darvin
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(darvin SHARED ${PROJECT_SOURCES})
    else()
        add_executable(darvin ${PROJECT_SOURCES})
    endif()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(darvin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# ── Fix: __imp___argc undefined reference ──────────────────────────────────
# Qt6EntryPoint wraps WinMain and references CRT symbols like __argc/__argv.
# When the runner's MinGW version doesn't exactly match Qt's build toolchain
# (e.g. CI uses GCC 15.2 but Qt 6.8 was built with a different MinGW),
# those CRT symbols are missing → linker error.
#
# Solution: skip Qt6EntryPoint entirely with QT_NO_ENTRYPOINT and tell the
# linker directly to use the Windows subsystem.  Qt's QTRY_GENERATE_MAIN
# macro in main.cpp still fires correctly.
if(WIN32)
    target_compile_definitions(darvin PRIVATE QT_NO_ENTRYPOINT)
    target_link_options(darvin PRIVATE -Wl,-subsystem,windows)
endif()
# ───────────────────────────────────────────────────────────────────────────

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.darvin)
endif()

set_target_properties(darvin PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS darvin
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(darvin)
endif()