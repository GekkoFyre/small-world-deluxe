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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR) # This version is needed to properly search for libpthreads

# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Useful-Variables#system-compiler-information
if(${CMAKE_VERSION} VERSION_GREATER "3.15.0")
    cmake_policy(SET CMP0091 NEW)
endif()

cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0054 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

project(smallworld C CXX)

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

set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/";
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/third-party/basic-c-sdl-project/cmake/sdl2/")

option(GFYRE_PACKAGE_MAINTAINER "Are you a package maintainer? If so, set this option to `ON`! It'll enable specific build settings to make your life easier." ON)
# option(GFYRE_BUILD_STATIC "Build Small World Deluxe as a static library/package. This will result in a larger executable at the benefit of requiring less DLL dependencies.")
# option(GFYRE_BUILD_SHARED "Build Small World Deluxe as a shared library/package. This will result in a smaller executable at the cost of requiring further DLL dependencies.")

#
# Detect if we are dealing with the Linux operating system and not Apple OS/X
#
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

option(GFYRE_ENABLE_PVS_STUDIO "Enable the static analysis of the Small World Deluxe project via PVS-Studio. More details can be found here: https://github.com/viva64/pvs-studio-cmake-examples" OFF)

#
# Determine the current version of Microsoft Windows in use, if that is indeed the operating system.
# Then proceed to build specifically for that operating system, provided the right flags have been set.
# https://stackoverflow.com/questions/9742003/platform-detection-in-cmake
#
if(WIN32 OR MSYS OR MINGW)
    if(${GFYRE_PACKAGE_MAINTAINER} EQUAL OFF)
        macro(get_WIN32_WINNT version)
            if (WIN32 OR MSYS OR MINGW AND CMAKE_SYSTEM_VERSION)
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
if (LINUX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if (Threads_FOUND)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
else()
    message(FATAL_ERROR "Could not find the Threads library!")
endif(Threads_FOUND)

find_package(Qt5 COMPONENTS Core Gui Widgets PrintSupport Test REQUIRED)

set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Use static libraries when linking Boost C++.")
set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multithreaded libraries when linking Boost C++.")
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime when linking Boost C++.")
find_package(Boost 1.54.0 REQUIRED COMPONENTS "filesystem"
                                              "system"
                                              "exception"
                                              "thread"
                                              "chrono"
                                              "locale"
                                              "random")
                                              
if(WIN32 OR MSYS OR MINGW)
    if(${GFYRE_PACKAGE_MAINTAINER} EQUAL ON)
        message(STATUS "Using package maintainer settings.")
        if (MSYS OR MINGW OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            # http://stackoverflow.com/questions/9742003/platform-detection-in-cmake
            add_definitions(-D_WIN32_WINNT=NTDDI_VISTASP1)
        endif()
    endif()
endif()

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

#
# Don't forget to include SDLmain.h and SDLmain.m your project for the
# OS X framework based version.  (Other versions link to -lSDLmain which
# this module will try to find on your behalf.) Also for OS X, this
# module will automatically add the -framework Cocoa on your behalf.
#
if (LINUX OR MSYS OR MINGW)
    find_package(SDL2 REQUIRED)
    if (SDL2MAIN_FOUND AND SDL2_VERSION_STRING MATCHES "2.*.*")
        include_directories(${SDL2_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${SDL2_LIBRARIES})
    else()
        message(SEND_ERROR "The cross-platform Simple DirectMedia Layer (libSDL2) library could not be found!")
    endif()
endif()

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
    if(WIN32 OR MSYS OR MINGW)
        # Enable `PA_USE_ASIO` by default, since this allows for the lowest latency possible with the audio pathways!
        option(PA_USE_ASIO "Enable support for ASIO. This is recommended for Microsoft Windows operating systems." ON)
    else()
        # Disable `PA_USE_ASIO` because we are dealing with a system other than Microsoft Windows, where ASIO is unsupported...
        option(PA_USE_ASIO "Disable support for ASIO. This is recommended for operating systems other than Microsoft Windows." OFF)
    endif()

    if (PA_USE_ASIO)
        add_definitions(-DPA_USE_ASIO=ON)
    else()
        add_definitions(-DPA_USE_ASIO=OFF)
    endif()

    include_directories(${PortAudio_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PortAudio_LIBRARIES})
else()
    message(FATAL_ERROR "The 'PortAudio' sound library could not be found!")
endif(PortAudio_FOUND)

find_package(PortAudioCpp REQUIRED)
if (PortAudioCpp_FOUND)
    include_directories(${PortAudioCpp_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${PortAudioCpp_LIBRARIES})
else()
    message(FATAL_ERROR "The C++ bindings for PortAudio could not be found! You must compile this separately from the code found within: 'src\contrib\portaudio\cpp'.")
endif(PortAudioCpp_FOUND)

find_package(Hamlib REQUIRED)
if (HAMLIB_FOUND)
    include_directories(${HAMLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${HAMLIB_LIBRARIES})
    ADD_DEFINITIONS(-DUSE_HAMLIB=1)
else()
    message(FATAL_ERROR "The 'Hamlib' amateur radio library could not be found!")
endif(HAMLIB_FOUND)

find_package(USB REQUIRED)
if (USB_FOUND)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LIBUSB_LIBRARIES})
else()
    message(FATAL_ERROR "The 'libusb' USB system library could not be found!")
endif(USB_FOUND)

find_package(FFTW REQUIRED)
if (FFTW_FOUND)
    include_directories(${FFTW_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${FFTW_LIBRARIES})
else()
    message(SEND_ERROR "The 'FFTW' mathematics library could not be found!")
endif(FFTW_FOUND)

find_package(QWT REQUIRED)
if (QWT_FOUND)
    include_directories(${QWT_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${QWT_LIBRARIES})
else()
    message(SEND_ERROR "The 'QWT' graphing/charting library could not be found!")
endif(QWT_FOUND)

#
# https://xiph.org/vorbis/
#
find_package(Vorbis REQUIRED)
if (Vorbis_FOUND)
    include_directories(${Vorbis_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Vorbis_LIBRARIES})
else()
    message(SEND_ERROR "The 'Vorbis' audio (de-/en-)coding library could not be found!")
endif(Vorbis_FOUND)

#
# https://xiph.org/vorbis/
#
find_package(Ogg REQUIRED)
if (Ogg_FOUND)
    include_directories(${Ogg_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Ogg_LIBRARIES})
else()
    message(SEND_ERROR "The 'Ogg' audio (de-/en-)coding library could not be found!")
endif(Ogg_FOUND)

#
# http://opus-codec.org/
#
find_package(Opus REQUIRED)
if (Opus_FOUND)
    include_directories(${Opus_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Opus_LIBRARIES})
else()
    message(SEND_ERROR "The 'Opus' audio (de-/en-)coding library could not be found!")
endif(Opus_FOUND)

find_package(Iconv REQUIRED)
if (Iconv_FOUND)
    include_directories(${Iconv_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Iconv_LIBRARIES})
else()
    message(SEND_ERROR "The 'libiconv' character encoding and conversion library could not be found!")
endif(Iconv_FOUND)

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")

#
# Codec2 library
#

#
# Set project version information. This should probably be done via external
# file at some point.
#
set(CODEC2_VERSION_MAJOR 0)
set(CODEC2_VERSION_MINOR 9)
# Set to patch level if needed, otherwise leave FALSE.
# Must be positive (non-zero) if set, since 0 == FALSE in CMake.
set(CODEC2_VERSION_PATCH 2)
set(CODEC2_VERSION "${CODEC2_VERSION_MAJOR}.${CODEC2_VERSION_MINOR}")
# Patch level version bumps should not change API/ABI.
set(SOVERSION "${CODEC2_VERSION_MAJOR}.${CODEC2_VERSION_MINOR}")
if(CODEC2_VERSION_PATCH)
    set(CODEC2_VERSION "${CODEC2_VERSION}.${CODEC2_VERSION_PATCH}")
endif()
message(STATUS "codec2 version: ${CODEC2_VERSION}")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/codec2/src")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/codec2/unittest")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/codec2/misc")

# set up a mapping so that the Release configuration for the Qt imported target is
# used in the COVERAGE CMake configuration.
set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

qt5_wrap_ui(UI_HEADERS
    src/ui/mainwindow.ui
    src/ui/dialogsettings.ui
    src/ui/aboutdialog.ui
    src/ui/spectrodialog.ui
    src/ui/gkaudioplaydialog.ui)

qt5_add_resources(UI_RESOURCES src/sworld_res.qrc)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

set(GALAXY_UI_CPP
    ${GALAXY_UI_CPP}
    src/ui/mainwindow.cpp
    src/ui/dialogsettings.cpp
    src/ui/aboutdialog.cpp
    src/ui/gkaudioplaydialog.cpp
    src/ui/spectrodialog.cpp
    src/spectro_fftw.cpp
    src/radiolibs.cpp
    src/dek_db.cpp
    src/file_io.cpp
    src/audio_devices.cpp
    src/pa_audio_buf.cpp
    src/spectro_gui.cpp
    src/pa_mic.cpp
    src/pa_mic_background.cpp
    src/gk_audio_encoding.cpp
    src/gk_audio_decoding.cpp
    src/gk_cli.cpp
    src/gk_modem.cpp
    src/contrib/RS232/rs232.c)

if(WIN32 OR MSYS OR MINGW)
    set(GALAXY_UI_CPP
        ${GALAXY_UI_CPP}
        src/string_funcs_windows.cpp)
elseif(LINUX)
    set(GALAXY_UI_CPP
        ${GALAXY_UI_CPP}
        src/string_funcs_linux.cpp)
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

set(GALAXY_SRC_CPP
    ${GALAXY_SRC_CPP}
    src/pa_sinewave.cpp)

set(GALAXY_SRC_HEADERS
    ${GALAXY_SRC_HEADERS}
    src/defines.hpp
    src/ui/mainwindow.hpp
    src/ui/dialogsettings.hpp
    src/ui/aboutdialog.hpp
    src/ui/gkaudioplaydialog.hpp
    src/ui/spectrodialog.hpp
    src/pa_sinewave.hpp
    src/spectro_fftw.hpp
    src/radiolibs.hpp
    src/dek_db.hpp
    src/file_io.hpp
    src/audio_devices.hpp
    src/pa_audio_buf.hpp
    src/spectro_gui.hpp
    src/pa_mic.hpp
    src/pa_mic_background.hpp
    src/gk_audio_encoding.hpp
    src/gk_audio_decoding.hpp
    src/gk_cli.hpp
    src/gk_modem.hpp
    src/contrib/RS232/rs232.h)

if(WIN32 OR MSYS OR MINGW)
    set(GALAXY_SRC_HEADERS
        ${GALAXY_SRC_HEADERS}
        src/string_funcs_windows.hpp)
elseif(LINUX)
    set(GALAXY_SRC_HEADERS
        ${GALAXY_SRC_HEADERS}
        src/string_funcs_linux.hpp)
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

add_definitions("-DQWT_DLL")

#
# Check that the following for the applicable systems actually exist! Abort otherwise...
# https://cmake.org/cmake/help/latest/module/CheckSymbolExists.html
# https://cmake.org/cmake/help/latest/module/CheckCXXSymbolExists.html
#
# include(CheckSymbolExists)
# include(CheckCXXSymbolExists)
#
include(CheckIncludeFile)
include(CheckIncludeFileCXX)

if(WIN32 OR MSYS OR MINGW OR LINUX OR UNIX)
    check_include_file("stdio.h" HAVE_STDIO_H)
    check_include_file("string.h" HAVE_CSTRING_H)
    check_include_file("stdlib.h" HAVE_STDLIB_H)
    check_include_file("stdint.h" HAVE_STDINT_H)
    check_include_file("time.h" HAVE_CTIME_H)
    check_include_file("math.h" HAVE_CMATH_H)
endif()

if(WIN32)
    check_include_file_cxx("atlbase.h" HAVE_ATLBASE_H)
    check_include_file_cxx("atlstr.h" HAVE_ATLSTR_H)
endif()

if(WIN32 OR MSYS OR MINGW)
    check_include_file_cxx("Windows.h" HAVE_WINDOWS_H)
    check_include_file_cxx("winsdkver.h" HAVE_WINSDKVER_H)
    check_include_file_cxx("tchar.h" HAVE_TCHAR_H)
    check_include_file_cxx("stringapiset.h" HAVE_STRINGAPISET_H)
endif()

if(LINUX OR MSYS OR MINGW)
    check_include_file("termios.h" HAVE_TERMIOS_H)
    check_include_file("unistd.h" HAVE_UNISTD_H)
    check_include_file("fcntl.h" HAVE_FCNTL_H)
    check_include_file("limits.h" HAVE_LIMITS_H)
    check_include_file("errno.h" HAVE_ERRNO_H)
    check_include_file("sys/ioctl.h" HAVE_IOCTL_H)
    check_include_file("sys/types.h" HAVE_TYPES_H)
    check_include_file("sys/stat.h" HAVE_STAT_H)
    check_include_file("sys/file.h" HAVE_FILE_H)
endif()

if(LINUX)
    check_include_file("linux/serial.h" HAVE_LSERIAL_H)
endif()

#
# The below is required for when we decide to start programming an install target for this CMake script!
#
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

add_library(galaxy STATIC ${GALAXY_SRC_CPP} ${GALAXY_UI_CPP} ${UI_MOC} ${GALAXY_SRC_HEADERS} ${UI_HEADERS})
set_property(TARGET galaxy PROPERTY AUTOMOC ON)
target_link_libraries(galaxy Qt5::Widgets Qt5::Core Qt5::Gui Qt5::PrintSupport Threads::Threads ${LIBS})

# rcc -no-compress -binary smallworld_res.qrc -o smallworld_res.rcc
qt5_add_binary_resources(resources src/sworld_res.qrc OPTIONS -no-compress)
add_dependencies(galaxy resources)

set(SWRLD_EXEC ${SWRLD_EXEC}
    src/main.cpp)

add_executable(smallworld ${SWRLD_EXEC})
target_link_libraries(smallworld galaxy)

#
# https://stackoverflow.com/questions/23995019/what-is-the-modern-method-for-setting-general-compile-flags-in-cmake
# https://stackoverflow.com/questions/20105167/c-error-failure-localefacet-s-create-c-locale-name-not-valid-when-runni
#
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Using regular Clang or AppleClang
    target_compile_options(galaxy PUBLIC -Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context -enable-clocale)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Using GCC
    target_compile_options(galaxy PUBLIC -Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context -enable-clocale)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # Using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Using Microsoft Visual Studio C++
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(galaxy PUBLIC "/W1 /MP")
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(galaxy PUBLIC "/W1 /GL /MP")
    endif()
endif()

if (GFYRE_ENABLE_PVS_STUDIO)
    include(PVS-Studio.cmake)
    pvs_studio_add_target(TARGET smallworld.analyze ALL
                          OUTPUT FORMAT errorfile
                          ANALYZE smallworld galaxy
                          LOG swd.err)
    message(STATUS "Enabled static analysis with PVS-Studio!")
else()
    message(STATUS "Disabled static analysis with PVS-Studio!")
endif()