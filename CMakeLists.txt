cmake_minimum_required(VERSION 3.16)
project(openkj VERSION 2.0 LANGUAGES C CXX)

# Force C++17 to prevent Qt 5.15 stdext errors under MSVC C++20
set(CMAKE_CXX_STANDARD 17)
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

# GStreamer Configuration
if(WIN32)
    # Prefer MSVC installation path set by environment variable or standard locations
    if(DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
        set(GSTREAMER_ROOT "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}")
    elseif(EXISTS "C:/gstreamer/1.0/msvc_x86_64")
        set(GSTREAMER_ROOT "C:/gstreamer/1.0/msvc_x86_64")
    else()
        set(GSTREAMER_ROOT "C:/Program Files/gstreamer/1.0/msvc_x86_64")
    endif()

    set(GSTREAMER_INCLUDE_DIRS
        "${GSTREAMER_ROOT}/include/gstreamer-1.0"
        "${GSTREAMER_ROOT}/lib/gstreamer-1.0/include"
        "${GSTREAMER_ROOT}/include/glib-2.0"
        "${GSTREAMER_ROOT}/lib/glib-2.0/include"
        "${GSTREAMER_ROOT}/include/glib-2.0/gobject"
    )
    
    set(GSTREAMER_LIBRARIES
        "${GSTREAMER_ROOT}/lib/gstreamer-1.0.lib"
        "${GSTREAMER_ROOT}/lib/glib-2.0.lib"
        "${GSTREAMER_ROOT}/lib/gobject-2.0.lib"
        "${GSTREAMER_ROOT}/lib/gstapp-1.0.lib"
        "${GSTREAMER_ROOT}/lib/gstvideo-1.0.lib"
        "${GSTREAMER_ROOT}/lib/gstaudio-1.0.lib"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GSTREAMER REQUIRED 
        gstreamer-1.0 
        gstreamer-app-1.0 
        gstreamer-video-1.0 
        gstreamer-audio-1.0 
        glib-2.0 
        gobject-2.0
    )
endif()

# Fetch external dependency spdlog
include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Recursively collect all source files inside src/
file(GLOB_RECURSE OPENKJ_SOURCES 
    "src/*.cpp" 
    "src/*.h" 
    "src/*.hpp" 
    "src/*.ui" 
    "src/resources.qrc"
)

# Add main executable target
add_executable(openkj ${OPENKJ_SOURCES})

# Target Include Directories
target_include_directories(openkj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GSTREAMER_INCLUDE_DIRS}
)

# Target Link Libraries
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
