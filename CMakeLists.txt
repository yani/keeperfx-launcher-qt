cmake_minimum_required(VERSION 3.24)

# Project settings
project(keeperfx-launcher-qt VERSION 0.1 LANGUAGES CXX)

# Check for supported operating systems
if(NOT (LINUX OR WINDOWS))
    message(FATAL_ERROR "Only Linux and Windows compilation is implemented")
endif()

# Force static linking on Windows
# set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
# set(BUILD_SHARED_LIBS OFF)
# set(CMAKE_EXE_LINKER_FLAGS "-static")

# Define LIEF_STATIC so that LIEF's headers assume static linking.
# add_compile_definitions(LIEF_STATIC)

# CMake settings
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# LIEF dependency
# if(LINUX)
#     set(LIEF_INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/LIEF-0.17.0-Linux-x86_64)
#     if(NOT EXISTS LIEF_INSTALL_DIR)
#         download_and_extract("https://lief.s3-website.fr-par.scw.cloud/latest/sdk/LIEF-0.17.0-Linux-x86_64.tar.gz" ${CMAKE_SOURCE_DIR}/deps)
#     endif()
# elseif(WINDOWS)
#     set(LIEF_INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/LIEF-0.17.0-win64)
#     if(NOT EXISTS LIEF_INSTALL_DIR)
#         download_and_extract("https://lief.s3-website.fr-par.scw.cloud/latest/sdk/LIEF-0.17.0-win64.zip" ${CMAKE_SOURCE_DIR}/deps)
#     endif()
# endif()
# set(LIEF_DIR ${LIEF_INSTALL_DIR}/lib/cmake/LIEF)

# Add LIEF to CMAKE_PREFIX_PATH
# set(LIEF_DIR "${LIEF_INSTALL_DIR}/share/LIEF/cmake")
# option(LIEF_DOC "Build LIEF docs" OFF)
# option(LIEF_PYTHON_API "Build LIEF Python API" OFF)
# option(LIEF_EXAMPLES "Build LIEF examples" OFF)
# option(LIEF_TESTS "Build LIEF tests" OFF)
# add_subdirectory(${LIEF_INSTALL_DIR})

# bit7z
set(BIT7Z_DIR ${CMAKE_SOURCE_DIR}/deps/bit7z-4.0.9/)
if(NOT EXISTS BIT7Z_DIR)
    download_and_extract("https://github.com/rikyoz/bit7z/archive/refs/tags/v4.0.9.zip" ${CMAKE_SOURCE_DIR}/deps)
endif()
add_subdirectory(${BIT7Z_DIR})

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui)
find_package(ZLIB REQUIRED)
find_package(LIEF REQUIRED)

# # LIEF
# set(LIEF_INCLUDE_DIR ${LIEF_INSTALL_DIR}/include)
# set(LIEF_LIB_DIR ${LIEF_INSTALL_DIR}/lib)
# include_directories(${LIEF_INCLUDE_DIR})
# link_directories(${LIEF_LIB_DIR})

# Get source files
file(GLOB PROJECT_SOURCES src/*.cpp src/*.h)
file(GLOB UI_FILES ui/*.ui)

# Add files to executable
qt_add_executable(keeperfx-launcher-qt
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${UI_FILES}
    resources.qrc
)

# Link libraries
target_link_libraries(
    keeperfx-launcher-qt
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
    bit7z
    ZLIB::ZLIB
    # ${LIEF_LIB_DIR}/LIEF.lib
    LIEF::LIEF
)

# Static linking of libLIEF
# if(LINUX)
#     target_link_libraries(keeperfx-launcher-qt PRIVATE ${LIEF_LIB_DIR}/libLIEF.a)
# elseif(WINDOWS)
#     target_link_libraries(keeperfx-launcher-qt PRIVATE ${LIEF_LIB_DIR}/LIEF.lib)
# endif()

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

# Copy 7z library
if (LINUX)
    add_custom_command(TARGET keeperfx-launcher-qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/ext/7z.so
        $<TARGET_FILE_DIR:keeperfx-launcher-qt>)
elseif (WINDOWS)
    add_custom_command(TARGET keeperfx-launcher-qt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/ext/7za.dll
        $<TARGET_FILE_DIR:keeperfx-launcher-qt>)
endif()
