cmake_minimum_required(VERSION 3.24)

# Project settings
project(keeperfx-launcher-qt VERSION 0.1 LANGUAGES CXX)

# Check for supported operating systems
if(NOT (UNIX OR WIN32))
    message(FATAL_ERROR "Only UNIX and WIN32 compilation is implemented")
endif()

# CMake settings
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MinGW/GCC Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -static -Os -s")
endif()

# Set custom UI directory
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)

# Create /deps dir if it does not exist yet
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/deps")

# Function to download and extract dependencies
function(download_and_extract url output_dir)
    file(DOWNLOAD ${url} ${CMAKE_BINARY_DIR}/temp_archive)
    if(UNIX)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/temp_archive WORKING_DIRECTORY ${output_dir})
    elseif(WIN32)
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/temp_archive WORKING_DIRECTORY ${output_dir})
    endif()
    file(REMOVE ${CMAKE_BINARY_DIR}/temp_archive)
endfunction()

# bit7z
set(BIT7Z_DIR ${CMAKE_SOURCE_DIR}/deps/bit7z-4.0.9/)
if(NOT EXISTS BIT7Z_DIR)
    download_and_extract("https://github.com/rikyoz/bit7z/archive/refs/tags/v4.0.9.zip" ${CMAKE_SOURCE_DIR}/deps)
endif()
add_subdirectory(${BIT7Z_DIR})

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui)

# Add Zlib
find_package(ZLIB)
if (NOT ZLIB_FOUND)
    message(STATUS "Zlib library not found, using local dependency")
    set(ZLIB_DIR ${CMAKE_SOURCE_DIR}/deps/zlib-1.3.1)
    if(NOT EXISTS ${ZLIB_DIR})
        download_and_extract("https://zlib.net/zlib131.zip" ${CMAKE_SOURCE_DIR}/deps)
    endif()
    add_subdirectory(${ZLIB_DIR})
endif()

# Add LIEF
find_package(LIEF)
if (NOT LIEF_FOUND)
    message(STATUS "LIEF library not found, using local dependency")

    # Download LIEF if it is not downloaded yet
    if(UNIX)
        set(LIEF_INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/LIEF-0.17.0-Linux-x86_64)
        if(NOT EXISTS LIEF_INSTALL_DIR)
            download_and_extract("https://lief.s3-website.fr-par.scw.cloud/latest/sdk/LIEF-0.17.0-Linux-x86_64.tar.gz" ${CMAKE_SOURCE_DIR}/deps)
        endif()
    elseif(WIN32)
        set(LIEF_INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/LIEF-0.17.0-win64)
        if(NOT EXISTS LIEF_INSTALL_DIR)
            download_and_extract("https://lief.s3-website.fr-par.scw.cloud/latest/sdk/LIEF-0.17.0-win64.zip" ${CMAKE_SOURCE_DIR}/deps)
        endif()
    endif()

    # Load local LIEF
    set(LIEF_INCLUDE_DIR ${LIEF_INSTALL_DIR}/include)
    set(LIEF_LIB_DIR ${LIEF_INSTALL_DIR}/lib)
    include_directories(${LIEF_INCLUDE_DIR})
    link_directories(${LIEF_LIB_DIR})
endif()

# Get source files
file(GLOB PROJECT_SOURCES src/*.cpp src/*.h)
file(GLOB UI_FILES ui/*.ui)

if(WIN32)
    qt_add_executable(keeperfx-launcher-qt
        MANUAL_FINALIZATION
        WIN32
        ${PROJECT_SOURCES}
        ${UI_FILES}
        resources.qrc
        app.rc
    )
else()
    qt_add_executable(keeperfx-launcher-qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${UI_FILES}
        resources.qrc
    )
endif()

# Link libraries
target_link_libraries(
    keeperfx-launcher-qt
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    bit7z
)

# Link Zlib
if (ZLIB_FOUND)
    target_link_libraries(keeperfx-launcher-qt PRIVATE ZLIB::ZLIB)
else()
    target_include_directories(keeperfx-launcher-qt PRIVATE ${ZLIB_DIR})
    target_link_libraries(keeperfx-launcher-qt PRIVATE zlibstatic)
endif()

# Link LIEF
if (LIEF_FOUND)
    target_link_libraries(keeperfx-launcher-qt PRIVATE LIEF::LIEF)
else()
    if(UNIX)
        target_link_libraries(keeperfx-launcher-qt PRIVATE ${LIEF_LIB_DIR}/libLIEF.a)
    elseif(WIN32)
        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            target_link_libraries(keeperfx-launcher-qt PRIVATE ${LIEF_LIB_DIR}/LIEF_static.lib)
        else()
            target_link_libraries(keeperfx-launcher-qt PRIVATE ${LIEF_LIB_DIR}/LIEF.lib)
        endif()
    endif()
endif()

# Set target properties
set_target_properties(keeperfx-launcher-qt PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Installation settings
include(GNUInstallDirs)
install(TARGETS keeperfx-launcher-qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Finalize executable
qt_finalize_executable(keeperfx-launcher-qt)

# Copy pre-compiled libraries
if (UNIX)
    file(GLOB COMPILED_LIBS "${CMAKE_SOURCE_DIR}/ext/*.so")
elseif (WIN32)
    file(GLOB COMPILED_LIBS "${CMAKE_SOURCE_DIR}/ext/*.dll")
endif()
foreach(LIB ${COMPILED_LIBS})
    add_custom_command(TARGET keeperfx-launcher-qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${LIB}" $<TARGET_FILE_DIR:keeperfx-launcher-qt>
        VERBATIM
    )
endforeach()
