cmake_minimum_required(VERSION 3.16)
project(OpenKJ LANGUAGES C CXX)

# Enforce C++17 to prevent Qt 5.15 stdext errors under MSVC
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt5 Dependencies
find_package(Qt5 COMPONENTS Core Gui Widgets Sql Network Svg PrintSupport Concurrent REQUIRED)

# GStreamer Setup for Windows/MSVC
if(WIN32)
    set(GSTREAMER_ROOT "C:/gstreamer/1.0/msvc_x86_64" CACHE PATH "Path to GStreamer MSVC installation")
    if(NOT EXISTS "${GSTREAMER_ROOT}")
        set(GSTREAMER_ROOT "C:/Program Files/gstreamer/1.0/msvc_x86_64")
    endif()

    include_directories(
        "${GSTREAMER_ROOT}/include/gstreamer-1.0"
        "${GSTREAMER_ROOT}/lib/gstreamer-1.0/include"
        "${GSTREAMER_ROOT}/include/glib-2.0"
        "${GSTREAMER_ROOT}/lib/glib-2.0/include"
    )
    
    link_directories(
        "${GSTREAMER_ROOT}/lib"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
    include_directories(${GSTREAMER_INCLUDE_DIRS})
    link_directories(${GSTREAMER_LIBRARY_DIRS})
endif()

# Source files & Target setup
add_subdirectory(src)
