cmake_minimum_required(VERSION 3.5)

project(keeperfx-launcher-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(USE_QT5 OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Gui)
find_package(ZLIB REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/deps/bit7z-4.0.8/)

# Set custom UI directory
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

# Set the path to the LIEF library and include directory
set(LIEF_DIR ${CMAKE_SOURCE_DIR}/deps/LIEF-0.15.1-Linux-x86_64)
set(LIEF_INCLUDE_DIR ${LIEF_DIR}/include)
set(LIEF_LIB_DIR ${LIEF_DIR}/lib)

# Add the LIEF include directory
include_directories(${LIEF_INCLUDE_DIR})

# Specify the path to the LIEF shared library
link_directories(${LIEF_LIB_DIR})

file(GLOB PROJECT_SOURCES src/*.cpp src/*.h)
file(GLOB UI_FILES ui/*.ui)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(keeperfx-launcher-qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${UI_FILES}
    )
else()
    add_executable(keeperfx-launcher-qt
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(
    keeperfx-launcher-qt
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt6::Gui
    bit7z
    LIEF #linux only
    ZLIB::ZLIB
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.keeperfx-launcher-qt)
endif()
set_target_properties(keeperfx-launcher-qt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS keeperfx-launcher-qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(keeperfx-launcher-qt)
endif()

add_custom_command(TARGET keeperfx-launcher-qt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${LIEF_LIB_DIR}/libLIEF.so
    $<TARGET_FILE_DIR:keeperfx-launcher-qt>)
