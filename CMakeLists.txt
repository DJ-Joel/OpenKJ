cmake_minimum_required(VERSION 3.16)
project(openkj VERSION 2.0 LANGUAGES C CXX)

# Force C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt5 Components
find_package(Qt5 COMPONENTS 
    Core 
    Gui 
    Widgets 
    Sql 
    Network 
    Svg 
    PrintSupport 
    Concurrent 
    REQUIRED
)

# GStreamer Setup
if(WIN32)
    set(GST_POSSIBLE_ROOTS
        "C:/gstreamer/1.0/msvc_x86_64"
        "C:/gstreamer/1.0/x86_64"
        "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}"
        "$ENV{GSTREAMER_1_0_ROOT_X86_64}"
        "C:/Program Files/gstreamer/1.0/msvc_x86_64"
        "C:/Program Files/gstreamer/1.0/x86_64"
    )

    set(GSTREAMER_ROOT "")
    foreach(candidate ${GST_POSSIBLE_ROOTS})
        if(candidate AND EXISTS "${candidate}")
            set(GSTREAMER_ROOT "${candidate}")
            break()
        endif()
    endforeach()

    if(NOT GSTREAMER_ROOT)
        message(FATAL_ERROR "GStreamer installation directory not found! Check your MSI installation path.")
    endif()

    message(STATUS "Found GStreamer root at: ${GSTREAMER_ROOT}")

    set(GSTREAMER_INCLUDE_DIRS
        "${GSTREAMER_ROOT}/include/gstreamer-1.0"
        "${GSTREAMER_ROOT}/lib/gstreamer-1.0/include"
        "${GSTREAMER_ROOT}/include/glib-2.0"
        "${GSTREAMER_ROOT}/lib/glib-2.0/include"
        "${GSTREAMER_ROOT}/include/glib-2.0/gobject"
    )

    find_library(GST_LIB NAMES gstreamer-1.0 PATHS "${GSTREAMER_ROOT}/lib" "${GSTREAMER_ROOT}/lib/gstreamer-1.0" REQUIRED)
    find_library(GLIB_LIB NAMES glib-2.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GOBJECT_LIB NAMES gobject-2.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GST_APP_LIB NAMES gstapp-1.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GST_VIDEO_LIB NAMES gstvideo-1.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GST_AUDIO_LIB NAMES gstaudio-1.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GST_BASE_LIB NAMES gstbase-1.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GST_CONTROLLER_LIB NAMES gstcontroller-1.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)
    find_library(GST_PBUTILS_LIB NAMES gstpbutils-1.0 PATHS "${GSTREAMER_ROOT}/lib" REQUIRED)

    set(GSTREAMER_LIBRARIES
        ${GST_LIB}
        ${GLIB_LIB}
        ${GOBJECT_LIB}
        ${GST_APP_LIB}
        ${GST_VIDEO_LIB}
        ${GST_AUDIO_LIB}
        ${GST_BASE_LIB}
        ${GST_CONTROLLER_LIB}
        ${GST_PBUTILS_LIB}
        winmm
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED 
        gstreamer-1.0 
        gstreamer-app-1.0 
        gstreamer-video-1.0 
        gstreamer-audio-1.0 
        gstreamer-base-1.0
        gstreamer-controller-1.0
        gstreamer-pbutils-1.0
        glib-2.0 
        gobject-2.0
    )
endif()

# Fetch spdlog
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

if (APPLE)
    # Recent Xcode/AppleClang releases (Clang 21, Xcode 26.4+) tightened
    # consteval validation in a way that breaks compile-time format string
    # checking in the old fmt library bundled inside spdlog 1.12.0. That old
    # fmt predates the FMT_USE_CONSTEVAL escape hatch entirely - its
    # FMT_CONSTEVAL detection logic has a genuine bug where an Apple Clang
    # exclusion only covers one of its two OR'd conditions, so modern Apple
    # Clang (which defines __cpp_consteval) slips through the other branch
    # anyway. Pre-defining FMT_CONSTEVAL as empty short-circuits that whole
    # detection block via its #ifndef guard, falling back to runtime format
    # string validation instead - functionally harmless.
    target_compile_definitions(spdlog PRIVATE "FMT_CONSTEVAL=")
endif ()

# Collect all source files inside src/
file(GLOB_RECURSE OPENKJ_SOURCES 
    "src/*.cpp" 
    "src/*.c" 
    "src/*.h" 
    "src/*.hpp" 
    "src/*.ui" 
    "src/resources.qrc"
)

# src/3rdparty contains a vendored copy of spdlog (including its own bench/,
# example/, and tests/ programs with their own main() functions and external
# dependencies like Google Benchmark). spdlog itself is supplied above via
# FetchContent, so exclude this vendored tree entirely from the build.
list(FILTER OPENKJ_SOURCES EXCLUDE REGEX "[/\\\\]src[/\\\\]3rdparty[/\\\\]")

# Main executable target
add_executable(openkj ${OPENKJ_SOURCES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Without this, CMake defaults to the console subsystem, which pops up a
    # terminal window alongside the GUI on every launch.
    set_property(
        TARGET openkj
        PROPERTY WIN32_EXECUTABLE TRUE
    )
endif ()

# taglib is vendored under src/taglib and built statically as part of this
# target (its .cpp files are already included via OPENKJ_SOURCES above).
# Its headers are spread across many nested subdirectories and use relative
# includes between them, so each needs to be on the include path directly.
target_compile_definitions(openkj PRIVATE TAGLIB_STATIC)

if (APPLE)
    # See the matching note above, near the spdlog FetchContent block - this
    # target also calls spdlog's templated logging macros directly, which
    # re-instantiates the same problematic fmt code path here too.
    target_compile_definitions(openkj PRIVATE "FMT_CONSTEVAL=")
endif ()

# Include Directories
target_include_directories(openkj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/toolkit
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mpeg/id3v2/frames
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mpeg/id3v1
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mod
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/ogg
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/ogg/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/ogg/opus
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/ogg/speex
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/ogg/vorbis
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/ape
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/riff
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/riff/aiff
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/riff/wav
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/asf
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/it
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mp4
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/mpc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/s3m
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/trueaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/wavpack
    ${CMAKE_CURRENT_SOURCE_DIR}/src/taglib/xm
    ${GSTREAMER_INCLUDE_DIRS}
)

# Link Libraries
target_link_libraries(openkj PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Sql
    Qt5::Network
    Qt5::Svg
    Qt5::PrintSupport
    Qt5::Concurrent
    spdlog::spdlog
    ${GSTREAMER_LIBRARIES}
)
