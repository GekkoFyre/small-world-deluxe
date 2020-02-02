#
#  ______  ______  ___   ___  ______  ______  ______  ______       
# /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\      
# \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \     
#  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_   
#   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \  
#    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \ 
#     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/ 
#                                                                 
#
#   If you have downloaded the source code for "Small World Deluxe" and are reading this,
#   then thank you from the bottom of our hearts for making use of our hard work, sweat
#   and tears in whatever you are implementing this into!
#
#   Copyright (C) 2020. GekkoFyre.
#
#   Small World Deluxe is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Small World is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Small World Deluxe.  If not, see <http://www.gnu.org/licenses/>.
#
#
#   The latest source code updates can be obtained from [ 1 ] below at your
#   discretion. A web-browser or the 'git' application may be required.
#
#   [ 1 ] - https://git.gekkofyre.io/amateur-radio/small-world-deluxe
#

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) # This version is needed to properly search for libpthreads

# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Useful-Variables#system-compiler-information
if(${CMAKE_VERSION} VERSION_GREATER "3.15.0")
    cmake_policy(SET CMP0091 NEW)
endif()

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0054 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(SmallWorld C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

#
# https://doc.qt.io/qt-5/cmake-manual.html
# https://cmake.org/cmake/help/v3.15/manual/cmake-qt.7.html#automoc
#
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# NOTE: Do not change this unless you know what you are doing!
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

option(GFYRE_PACKAGE_MAINTAINER "Are you a package maintainer? If so, set this option to `ON`! It'll enable specific build settings to make your life easier." ON)

if(WIN32)
    option(GFYRE_PA_USE_ASIO "Enable the ASIO sub-library option with PortAudio, which is recommended for Microsoft Windows systems <http://portaudio.com/docs/v19-doxydocs-dev/compile_windows_asio_msvc.html>." ON)
else()
    option(GFYRE_PA_USE_ASIO "Enable the ASIO sub-library option with PortAudio, which is recommended for Microsoft Windows systems <http://portaudio.com/docs/v19-doxydocs-dev/compile_windows_asio_msvc.html>." OFF)
endif()

#
# Determine the current version of Microsoft Windows in use, if that is indeed the operating system.
# Then proceed to build specifically for that operating system, provided the right flags have been set.
# https://stackoverflow.com/questions/9742003/platform-detection-in-cmake
#
if(WIN32)
    if(${GFYRE_PACKAGE_MAINTAINER} EQUAL OFF)
        macro(get_WIN32_WINNT version)
            if (WIN32 AND CMAKE_SYSTEM_VERSION)
                set(ver ${CMAKE_SYSTEM_VERSION})
                string(REPLACE "." "" ver ${ver})
                string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

                set(${version} "0x${ver}")
            endif()
        endmacro()
        
        get_WIN32_WINNT(ver)
        message(STATUS "Your Microsoft Windows has determined to be version: " ${ver})
        message(STATUS "If the WRONG version has been determined, please enable the variable: `{GFYRE_PACKAGE_MAINTAINER}")
        add_definitions(-D_WIN32_WINNT=${ver})
    endif(${GFYRE_PACKAGE_MAINTAINER} EQUAL OFF)
endif()

#
# https://stackoverflow.com/questions/33991918/link-to-pthread-library-using-cmake-in-clion
# Find 'libpthreads'
#
if (UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if (Threads_FOUND)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif(Threads_FOUND)

find_package(Qt5 COMPONENTS Core Gui Widgets Charts Test REQUIRED)

set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Use static libraries when linking Boost C++.")
set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multithreaded libraries when linking Boost C++.")
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime when linking Boost C++.")
find_package(Boost 1.54.0 REQUIRED COMPONENTS "filesystem"
                                              "system")
                                              
if(WIN32)
    if(${GFYRE_PACKAGE_MAINTAINER} EQUAL ON)
        message(STATUS "Using package maintainer settings.")
        if (${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR "cygwin" OR "mingw")
            # http://stackoverflow.com/questions/9742003/platform-detection-in-cmake
            add_definitions(-D_WIN32_WINNT=NTDDI_VISTASP1)
        endif()
    endif(${GFYRE_PACKAGE_MAINTAINER} EQUAL ON)
endif(WIN32)

if(${GFYRE_PACKAGE_MAINTAINER} EQUAL ON)
    set(BUILD_SHARED_LIBS ON)
endif(${GFYRE_PACKAGE_MAINTAINER} EQUAL ON)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})
    add_definitions(-DBOOST_ALL_NO_LIB)
    message(STATUS "Boost C++ libraries have been found.")
else()
message(FATAL_ERROR "The required 'Boost C++' libraries are either not installed, or not multithreaded and/or of not an up-to-date version. Boost C++ 1.54.0 is the minimally required version.")
endif(Boost_FOUND)

find_package(ZSTD REQUIRED)
if (ZSTD_FOUND)
    include_directories(${ZSTD_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${ZSTD_LIBRARIES})
else()
    message(SEND_ERROR "The 'ZSTD' compression library could not be found!")
endif(ZSTD_FOUND)

find_package(LZ4 REQUIRED)
if (LZ4_FOUND)
    include_directories(${LZ4_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LZ4_LIBRARIES})
else()
    message(SEND_ERROR "The 'LZ4' compression library could not be found!")
endif(LZ4_FOUND)

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${ZLIB_LIBRARIES})
else()
    message(SEND_ERROR "The 'ZLIB' compression library could not be found!")
endif(ZLIB_FOUND)

find_package(LevelDB REQUIRED)
if (LEVELDB_FOUND)
    include_directories(${LEVELDB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LEVELDB_LIBRARIES})
else()
    message(FATAL_ERROR "The 'LevelDB' key-value database library could not be found!")
endif(LEVELDB_FOUND)

find_package(PortAudio REQUIRED)
if (PortAudio_FOUND)
    if(GFYRE_PA_USE_ASIO)
        add_definitions(-DPA_USE_ASIO=1)
        message(STATUS "Enabled the `PA_USE_ASIO` build option!")
    endif()

    include_directories(${PortAudio_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PortAudio_LIBRARIES})
else()
    message(FATAL_ERROR "The 'PortAudio' sound library could not be found!")
endif(PortAudio_FOUND)

find_package(Hamlib REQUIRED)
if (HAMLIB_FOUND)
    include_directories(${HAMLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${HAMLIB_LIBRARIES})
    ADD_DEFINITIONS(-DUSE_HAMLIB=1)
else()
    message(FATAL_ERROR "The 'Hamlib' amateur radio library could not be found!")
endif(HAMLIB_FOUND)

find_package(USB REQUIRED)
if (LIBUSB_FOUND)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LIBUSB_LIBRARIES})
else()
    message(FATAL_ERROR "The 'libusb' USB system library could not be found!")
endif(LIBUSB_FOUND)

find_package(FFTW REQUIRED)
if (FFTW_FOUND)
    include_directories(${FFTW_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${FFTW_LIBRARIES})
else()
    message(SEND_ERROR "The 'FFTW' mathematics library could not be found!")
endif(FFTW_FOUND)

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")

# set up a mapping so that the Release configuration for the Qt imported target is
# used in the COVERAGE CMake configuration.
set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

#
# https://stackoverflow.com/questions/25989448/implementing-qt-project-through-cmake/25990278
#
if(WIN32)
    qt5_wrap_cpp(UI_MOC
        src/ui/mainwindow.hpp
        src/ui/dialogsettings.hpp
        src/ui/aboutdialog.hpp
        src/radiolibs.hpp
        src/dek_db.hpp
        src/file_io.hpp
        src/audio_devices.hpp
        src/spectro_fftw.hpp
        src/string_funcs_windows.hpp)
elseif(UNIX)
    qt5_wrap_cpp(UI_MOC
        src/ui/mainwindow.hpp
        src/ui/dialogsettings.hpp
        src/ui/aboutdialog.hpp
        src/radiolibs.hpp
        src/dek_db.hpp
        src/file_io.hpp
        src/audio_devices.hpp
        src/spectro_fftw.hpp
        src/string_funcs_linux.hpp)
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

qt5_wrap_ui(UI_HEADERS
    src/ui/mainwindow.ui
    src/ui/dialogsettings.ui
    src/ui/aboutdialog.ui)

# qt5_add_resources(UI_RESOURCES src/smallworld_res.qrc)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

if(WIN32)
    set(GALAXY_UI_CPP
        src/defines.hpp
        src/ui/mainwindow.cpp
        src/ui/dialogsettings.cpp
        src/ui/aboutdialog.cpp
        src/radiolibs.cpp
        src/dek_db.cpp
        src/file_io.cpp
        src/audio_devices.cpp
        src/spectro_fftw.cpp
        src/string_funcs_windows.cpp)
elseif(UNIX)
    set(GALAXY_UI_CPP
        src/defines.hpp
        src/ui/mainwindow.cpp
        src/ui/dialogsettings.cpp
        src/ui/aboutdialog.cpp
        src/radiolibs.cpp
        src/dek_db.cpp
        src/file_io.cpp
        src/audio_devices.cpp
        src/spectro_fftw.cpp
        src/string_funcs_linux.cpp)
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

add_library(galaxy STATIC ${GALAXY_UI_CPP} ${UI_MOC} ${UI_HEADERS})

target_link_libraries(galaxy Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Charts Qt5::Test Threads::Threads ${OpenCV_LIBS} ${LIBS})

# rcc -no-compress -binary smallworld_res.qrc -o smallworld_res.rcc
# qt5_add_binary_resources(resources src/smallworld_res.qrc OPTIONS -no-compress)
# add_dependencies(galaxy resources)

add_executable(smallworld
    src/main.cpp
    src/contrib/RS232/rs232.h
    src/contrib/RS232/rs232.c)

target_link_libraries(smallworld galaxy)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Using regular Clang or AppleClang
    target_compile_options(galaxy PUBLIC -Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Using GCC
    target_compile_options(galaxy PUBLIC -Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # Using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Using Microsoft Visual Studio C++
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(galaxy PUBLIC "/W1")
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(galaxy PUBLIC "/W1 /GL")
    endif()
endif()

