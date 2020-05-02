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
#   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
#

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) # This version is needed to properly search for libpthreads

# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Useful-Variables#system-compiler-information
if(${CMAKE_VERSION} VERSION_GREATER "3.15.0")
    cmake_policy(SET CMP0091 NEW)
endif()

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0054 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(PortAudioCpp C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

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

find_package(PortAudio REQUIRED)
if (PortAudio_FOUND)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSYS" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MINGW")
        # Enable `PA_USE_ASIO` by default, since this allows for the lowest latency possible with the audio pathways!
        if(GFYRE_PA_USE_ASIO)
            add_definitions(-DPA_USE_ASIO=1)
            message(STATUS "Enabled the `PA_USE_ASIO` build option!")
        endif()
    else()
        # Disable `PA_USE_ASIO` because we are dealing with a system other than Microsoft Windows, where ASIO is unsupported...
        add_definitions(-DPA_USE_ASIO=0)
        message(STATUS "Disabled the `PA_USE_ASIO` build option because we are not dealing with a supported Microsoft Windows system!")
    endif()

    include_directories(${PortAudio_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PortAudio_LIBRARIES})
else()
    message(FATAL_ERROR "The 'PortAudio' sound library could not be found!")
endif(PortAudio_FOUND)

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

if(WIN32)
    set(GALAXY_SRC_CPP
        src/BlockingStream.cxx
        src/CallbackInterface.cxx
        src/CallbackStream.cxx
        src/CFunCallbackStream.cxx
        src/CppFunCallbackStream.cxx
        src/Device.cxx
        src/DirectionSpecificStreamParameters.cxx
        src/Exception.cxx
        src/HostApi.cxx
        src/InterfaceCallbackStream.cxx
        src/MemFunCallbackStream.cxx
        src/Stream.cxx
        src/StreamParameters.cxx
        src/System.cxx
        src/SystemDeviceIterator.cxx
        src/SystemHostApiIterator.cxx)
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

if(WIN32)
    set(GALAXY_SRC_HEADERS
        include/portaudiocpp/AutoSystem.hxx
        include/portaudiocpp/BlockingStream.hxx
        include/portaudiocpp/CallbackInterface.hxx
        include/portaudiocpp/CallbackStream.hxx
        include/portaudiocpp/CFunCallbackStream.hxx
        include/portaudiocpp/CppFunCallbackStream.hxx
        include/portaudiocpp/Device.hxx
        include/portaudiocpp/DirectionSpecificStreamParameters.hxx
        include/portaudiocpp/Exception.hxx
        include/portaudiocpp/HostApi.hxx
        include/portaudiocpp/InterfaceCallbackStream.hxx
        include/portaudiocpp/MemFunCallbackStream.hxx
        include/portaudiocpp/PortAudioCpp.hxx
        include/portaudiocpp/SampleDataFormat.hxx
        include/portaudiocpp/Stream.hxx
        include/portaudiocpp/StreamParameters.hxx
        include/portaudiocpp/System.hxx
        include/portaudiocpp/SystemDeviceIterator.hxx
        include/portaudiocpp/SystemHostApiIterator.hxx)
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSYS" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MINGW")
    if(GFYRE_PA_USE_ASIO)
        set(GALAXY_SRC_HEADERS ${GALAXY_SRC_HEADERS}
        include/portaudiocpp/AsioDeviceAdapter.hxx)
        set(GALAXY_SRC_CPP ${GALAXY_SRC_CPP}
        src/AsioDeviceAdapter.cxx)
    endif()
endif()

add_library(portaudiocpp STATIC ${GALAXY_SRC_CPP} ${GALAXY_SRC_HEADERS})
target_include_directories(portaudiocpp PUBLIC include)
target_link_libraries(portaudiocpp ${LIBS})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Using regular Clang or AppleClang
    target_compile_options(portaudiocpp PUBLIC -Werror -Wall -Wextra)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Using GCC
    target_compile_options(portaudiocpp PUBLIC -Werror -Wall -Wextra)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # Using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Using Microsoft Visual Studio C++
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(portaudiocpp PUBLIC "/W1")
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(portaudiocpp PUBLIC "/W1 /GL")
    endif()
endif()
