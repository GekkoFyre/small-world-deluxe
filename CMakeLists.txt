#
#     __                 _ _   __    __           _     _ 
#    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
#    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
#    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
#    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
#                                                         
#                  ___     _                              
#                 /   \___| |_   ___  _____               
#                / /\ / _ \ | | | \ \/ / _ \              
#               / /_//  __/ | |_| |>  <  __/              
#              /___,' \___|_|\__,_/_/\_\___|              
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
cmake_policy(SET CMP0087 NEW)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

option(GK_CUDA_FFT "Enable NVIDIA CUDA support within Small World Deluxe and allow your GPU to do heavy calculations instead of your CPU!" OFF)

#
# RPATH is aimed at Linux installations primarily
#
set(CMAKE_INSTALL_RPATH $ORIGIN $ORIGIN/lib) # Relative path to lib-dir...
set(INSTALL_RPATH_USE_LINK_PATH true)

if(GK_CUDA_FFT)
    message(STATUS "Enable NVIDIA CUDA support within Small World Deluxe and allow your GPU to do heavy calculations instead of your CPU!")
    add_definitions(-DGK_CUDA_FFT_ENBL)
    project(smallworld C CXX Fortran CUDA)
else()
    project(smallworld C CXX Fortran)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

#
# Custom options specific to Small World Deluxe and/or GekkoFyre Networks <https://gekkofyre.io/>
#
option(GFYRE_PACKAGE_MAINTAINER "Are you a package maintainer? If so, set this option to `ON`! It'll enable specific build settings to make your life easier." ON)
option(GFYRE_BUILD_STATIC "Build Small World Deluxe as a static library/package. This will result in a larger executable at the benefit of requiring less DLL dependencies." ON)
option(GFYRE_BUILD_SHARED "Build Small World Deluxe as a shared library/package. This will result in a smaller executable at the cost of requiring further DLL dependencies." OFF)
option(GFYRE_SWORLD_DBG_VERBOSITY "Enable DEBUG verbosity for Small World Deluxe and its use of third-party libraries throughout the program itself." OFF)
option(GFYRE_PORTAUDIO_DBG_VERBOSITY "Enable DEBUG verbosity for the PortAudio library and its use throughout Small World Deluxe." OFF)

option(ENBL_VALGRIND_SUPPORT "Allows for the use of Valgrind memory analysis on the application, thereby disabling certain, otherwise needed features that enhance the user-experience." OFF)

option(BUILD_CODEC2_SUPPORT "Enable support for the 'Codec2' open source speech codec library." ON)
option(BUILD_OPUS_SUPPORT "Enable support for the 'Opus' codec library." OFF)

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
    add_definitions(-D_WIN32_WINNT=${ver})
endif()

#
# The below is required for when we decide to start programming an install target for this CMake script!
#
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if (LINUX)
    set(INCLUDE_INSTALL_DIR "/usr/local/include/small-world-deluxe/" CACHE PATH "Where Small World Deluxe itself should be installed.")
    set(LIB_INSTALL_DIR "/usr/local/lib/small-world-deluxe/" CACHE PATH "Location for the libraries/dependencies.")
    set(SYSCONFIG_INSTALL_DIR "/etc/small-world-deluxe/" CACHE PATH "Location for the settings directory.")
endif()

if (GFYRE_BUILD_STATIC)
    option(BUILD_SHARED_LIBS "Set this option to OFF in order to compile a static set of libraries." OFF)
else()
    option(BUILD_SHARED_LIBS "Set this option to OFF in order to compile a static set of libraries." ON)
endif(GFYRE_BUILD_STATIC)

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

find_package(Qt5 COMPONENTS Core Gui Widgets Network SerialPort PrintSupport TextToSpeech Test REQUIRED)

if (MSYS OR MINGW OR LINUX)
    set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Use static libraries when linking Boost C++.")
elseif(WIN32)
    set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Use static libraries when linking Boost C++.")
else()
    message(SEND_ERROR "The only operating systems and/or build environments we OFFICIALLY support as of the moment are Microsoft Windows, MinGW/MSYS, and Linux. Sorry!")
endif()

set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multithreaded libraries when linking Boost C++.")
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime when linking Boost C++.")
find_package(Boost 1.54.0 REQUIRED COMPONENTS "filesystem"
                                              "system"
                                              "exception"
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

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})
    add_definitions(-DBOOST_ALL_NO_LIB)
    message(STATUS "Boost C++ libraries have been found.")
else()
    message(FATAL_ERROR "The required 'Boost C++' libraries are either not installed, or not multithreaded and/or of not an up-to-date version. Boost C++ 1.54.0 is the minimally required version.")
endif(Boost_FOUND)

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${ZLIB_LIBRARIES})
else()
    message(FATAL_ERROR "The 'ZLIB' compression library could not be found!")
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
    if(LINUX OR MSYS OR MINGW)
        # Disable `PA_USE_ASIO` because we are dealing with a system other than Microsoft Windows, where ASIO is unsupported...
        option(PA_USE_ASIO "Disable support for ASIO. This is recommended for operating systems other than Microsoft Windows." OFF)
    elseif(WIN32)
        # Enable `PA_USE_ASIO` by default, since this allows for the lowest latency possible with the audio pathways!
        option(PA_USE_ASIO "Enable support for ASIO. This is recommended for Microsoft Windows operating systems." ON)
    elseif(APPLE)
        message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
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

find_package(Hamlib REQUIRED)
if (HAMLIB_FOUND)
    include_directories(${HAMLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${HAMLIB_LIBRARIES})
    ADD_DEFINITIONS(-DUSE_HAMLIB=1)

    find_package(HamlibCpp REQUIRED)
    if (HAMLIBCPP_FOUND)
        include_directories(${HAMLIBCPP_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${HAMLIBCPP_LIBRARIES})
    else()
        message(FATAL_ERROR "The 'Hamlib' amateur radio C++ library bindings could not be found!")
    endif(HAMLIBCPP_FOUND)
else()
    message(FATAL_ERROR "The 'Hamlib' amateur radio library could not be found!")
endif(HAMLIB_FOUND)

#
# Left blank intentionally!
#
set(GALAXY_CODEC2_CPP
    ${GALAXY_CODEC2_CPP})

#
# Left blank intentionally!
#
set(GALAXY_CODEC2_HPP
    ${GALAXY_CODEC2_HPP})

if (BUILD_CODEC2_SUPPORT)
    if (MSYS OR MINGW OR LINUX OR UNIX)
        message(STATUS "Support for 'Codec2' has been enabled! Finding package...")
        find_package(Codec2 REQUIRED)
        if (CODEC2_FOUND)
            include_directories(${CODEC2_INCLUDE_DIRS})
            set(LIBS ${LIBS} ${CODEC2_LIBRARIES})
            add_definitions("-DCODEC2_LIBS_ENBLD")
            
            set(GALAXY_CODEC2_CPP
                ${GALAXY_CODEC2_CPP}
                src/gk_codec2.cpp)
            
            set(GALAXY_CODEC2_HPP
                ${GALAXY_CODEC2_HPP}
                src/gk_codec2.hpp)
        else()
            message(FATAL_ERROR "The 'Codec2' open source speech codec library could not be found!")
        endif(CODEC2_FOUND)
    else()
        message(WARNING "Support for 'Codec2' is only possible with *nix systems or Macintosh OS/X! MinGW is another option if you're using Microsoft Windows.")
    endif()
endif(BUILD_CODEC2_SUPPORT)

find_package(Snappy REQUIRED)
if (SNAPPY_FOUND)
    include_directories(${SNAPPY_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${SNAPPY_LIBRARIES})
else()
    message(FATAL_ERROR "The 'Snappy' compression libraries could not be found!")
endif(SNAPPY_FOUND)

find_package(USB REQUIRED)
if (USB_FOUND)
    include_directories(${LIBUSB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LIBUSB_LIBRARIES})
    
    find_package(QtUsb REQUIRED)
    if (QtUsb_FOUND)
        include_directories(${QtUsb_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${QtUsb_LIBRARIES})
    else()
        message(FATAL_ERROR "The 'QtUSB' system device and enumeration library could not be found!")
    endif(QtUsb_FOUND)
else()
    message(FATAL_ERROR "Either `libusb.h` and/or `usb.h` could not be found for the 'libusb' system device and enumeration libraries!")
endif(USB_FOUND)

find_package(QWT REQUIRED)
if (QWT_FOUND)
    include_directories(${QWT_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${QWT_LIBRARIES})
else()
    message(FATAL_ERROR "The 'QWT' graphing/charting library could not be found!")
endif(QWT_FOUND)

if (LINUX)
    find_package(UDev REQUIRED)
    if (udev_FOUND)
        include_directories(${UDEV_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${UDEV_LIBRARIES})
    else()
        message(FATAL_ERROR "The 'udev' SystemD libraries could not be found!")
    endif(udev_FOUND)
endif(LINUX)

#
# https://xiph.org/vorbis/
#
find_package(Vorbis REQUIRED)
if (Vorbis_FOUND)
    include_directories(${Vorbis_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Vorbis_LIBRARIES})
else()
    message(FATAL_ERROR "The 'Vorbis' audio (de-/en-)coding library could not be found!")
endif(Vorbis_FOUND)

#
# https://xiph.org/vorbis/
#
find_package(Ogg REQUIRED)
if (Ogg_FOUND)
    include_directories(${Ogg_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Ogg_LIBRARIES})
else()
    message(FATAL_ERROR "The 'Ogg' audio (de-/en-)coding library could not be found!")
endif(Ogg_FOUND)

#
# http://opus-codec.org/
#
if (BUILD_OPUS_SUPPORT)
    message(STATUS "Support for 'Opus' has been enabled! Finding package...")
    find_package(Opus REQUIRED)
    if (Opus_FOUND)
        include_directories(${Opus_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${Opus_LIBRARIES})
        add_definitions("-DOPUS_LIBS_ENBLD")
    else()
        message(FATAL_ERROR "The 'Opus' audio (de-/en-)coding library could not be found!")
    endif(Opus_FOUND)
endif(BUILD_OPUS_SUPPORT)

find_package(Iconv REQUIRED)
if (Iconv_FOUND)
    include_directories(${Iconv_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Iconv_LIBRARIES})
else()
    message(FATAL_ERROR "The 'libiconv' character encoding and conversion library could not be found!")
endif(Iconv_FOUND)

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage -mwindows")

#
# Sentry (Native SDK for C/C++)
# https://github.com/getsentry/sentry-native
#
set(SENTRY_PIC ON CACHE BOOL "Build Sentry as a position independent library" FORCE)
set(SENTRY_EXPORT_SYMBOLS OFF CACHE BOOL "By default, sentry exposes all symbols in the dynamic symbol table. You might want to disable it in case the program intends to dlopen third-party shared libraries and avoid symbol collisions." FORCE)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(SENTRY_BUILD_RUNTIMESTATIC OFF CACHE BOOL "Disables linking with the static MSVC runtime. Has no effect if the compiler is not MSVC.")
endif()

if (LINUX)
    #
    # Currently Crashpad support on Linux via Sentry is very much unsupported as of the moment; see the link(s) down below...
    # https://github.com/getsentry/sentry-native
    # https://github.com/getsentry/sentry-native/issues/46
    #
    set(SENTRY_BACKEND "breakpad" CACHE STRING "The type of crash manager to use." FORCE)
    set(SENTRY_BREAKPAD_SYSTEM OFF CACHE BOOL "This instructs the build system to use system-installed breakpad or crashpad libraries instead of using the in-tree version. This is generally not recommended for crashpad, as sentry uses a patched version that has attachment support.")
else()
    #
    # Use Crashpad for all other operating systems, since support for everything but Linux is okay as of the moment (with the exception of Android but we have no plans for that right now)...
    #
    set(SENTRY_BACKEND "crashpad" CACHE STRING "The type of crash manager to use." FORCE)
    set(SENTRY_CRASHPAD_SYSTEM OFF CACHE BOOL "This instructs the build system to use system-installed breakpad or crashpad libraries instead of using the in-tree version. This is generally not recommended for crashpad, as sentry uses a patched version that has attachment support.")
endif()

set(SENTRY_LINK_PTHREAD ON CACHE BOOL "Links to the pthread library on Unix targets.")
set(SENTRY_BUILD_FORCE32 OFF CACHE BOOL "Otherwise forces cross-compilation from 64-bit host to 32-bit target. Only has an effect on Linux." FORCE)

if(WIN32 OR MSYS OR MINGW)
    set(SENTRY_TRANSPORT "winhttp" CACHE STRING "Please do not modify this." FORCE)
else()
    set(SENTRY_TRANSPORT "curl" CACHE STRING "Please do not modify this." FORCE)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/sentry-native")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/sentry-native/include")

#
# KissFFT
# https://github.com/mborgerding/kissfft
#
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/kissfft")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/kissfft")

#
# PortAudio's C++ bindings
#
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/portaudio/cpp/include")
set(GALAXY_PORTAUDIO_CPP
    ${GALAXY_PORTAUDIO_CPP}
    src/contrib/portaudio/cpp/src/BlockingStream.cxx
    src/contrib/portaudio/cpp/src/CallbackInterface.cxx
    src/contrib/portaudio/cpp/src/CallbackStream.cxx
    src/contrib/portaudio/cpp/src/CFunCallbackStream.cxx
    src/contrib/portaudio/cpp/src/CppFunCallbackStream.cxx
    src/contrib/portaudio/cpp/src/Device.cxx
    src/contrib/portaudio/cpp/src/DirectionSpecificStreamParameters.cxx
    src/contrib/portaudio/cpp/src/Exception.cxx
    src/contrib/portaudio/cpp/src/HostApi.cxx
    src/contrib/portaudio/cpp/src/InterfaceCallbackStream.cxx
    src/contrib/portaudio/cpp/src/MemFunCallbackStream.cxx
    src/contrib/portaudio/cpp/src/Stream.cxx
    src/contrib/portaudio/cpp/src/StreamParameters.cxx
    src/contrib/portaudio/cpp/src/System.cxx
    src/contrib/portaudio/cpp/src/SystemDeviceIterator.cxx
    src/contrib/portaudio/cpp/src/SystemHostApiIterator.cxx)

if(WIN32 OR MSYS OR MINGW)
    if(PA_USE_ASIO)
        set(GALAXY_PORTAUDIO_CPP
            ${GALAXY_PORTAUDIO_CPP}
            src/contrib/portaudio/cpp/src/AsioDeviceAdapter.cxx)
    endif()
endif()

set_property(SOURCE ${GALAXY_PORTAUDIO_CPP} PROPERTY SKIP_AUTOUIC ON)

set(GALAXY_WSJTX_QRA64_C
    ${GALAXY_WSJTX_QRA64_C}
    src/contrib/wsjtx/lib/ftrsd/int.h
    src/contrib/wsjtx/lib/qra/qra64/main.c
    src/contrib/wsjtx/lib/qra/qra64/fadengauss.c
    src/contrib/wsjtx/lib/qra/qra64/fadenlorentz.c
    src/contrib/wsjtx/lib/qra/qra64/qra64_subs.c
    src/contrib/wsjtx/lib/qra/qra64/qra64.c
    src/contrib/wsjtx/lib/qra/qra64/qra64.h
    src/contrib/wsjtx/lib/qra/qracodes/normrnd.c
    src/contrib/wsjtx/lib/qra/qracodes/normrnd.h
    src/contrib/wsjtx/lib/qra/qracodes/npfwht.c
    src/contrib/wsjtx/lib/qra/qracodes/npfwht.h
    src/contrib/wsjtx/lib/qra/qracodes/pdmath.c
    src/contrib/wsjtx/lib/qra/qracodes/pdmath.h
    src/contrib/wsjtx/lib/qra/qracodes/qra12_63_64_irr_b.c
    src/contrib/wsjtx/lib/qra/qracodes/qra12_63_64_irr_b.h
    src/contrib/wsjtx/lib/qra/qracodes/qra13_64_64_irr_e.c
    src/contrib/wsjtx/lib/qra/qracodes/qra13_64_64_irr_e.h
    src/contrib/wsjtx/lib/qra/qracodes/qracodes.c
    src/contrib/wsjtx/lib/qra/qracodes/qracodes.h)

set_property(SOURCE ${GALAXY_WSJTX_QRA64_C} PROPERTY SKIP_AUTOUIC ON)

set(GALAXY_WSJTX_WSPR_C
    ${GALAXY_WSJTX_WSPR_C}
    src/contrib/wsjtx/lib/wsprd/nhash.c
    src/contrib/wsjtx/lib/wsprd/wsprd_utils.c
    src/contrib/wsjtx/lib/wsprd/wsprd_utils.h
    src/contrib/wsjtx/lib/wsprd/fano.c
    src/contrib/wsjtx/lib/wsprd/fano.h
    src/contrib/wsjtx/lib/wsprd/jelinek.c
    src/contrib/wsjtx/lib/wsprd/jelinek.h)

set_property(SOURCE ${GALAXY_WSJTX_WSPR_C} PROPERTY SKIP_AUTOUIC ON)

#
# SingleApplication
# https://github.com/itay-grudev/SingleApplication
#
set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")
add_subdirectory(src/contrib/SingleApplication)

# set up a mapping so that the Release configuration for the Qt imported target is
# used in the COVERAGE CMake configuration.
set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")

qt5_wrap_ui(UI_HEADERS
    src/ui/mainwindow.ui
    src/ui/dialogsettings.ui
    src/ui/aboutdialog.ui
    src/ui/spectrodialog.ui
    src/ui/gkaudioplaydialog.ui
    src/ui/sendreportdialog.ui)

# https://doc.qt.io/qt-5/cmake-command-reference.html
# rcc -no-compress -binary smallworld_res.qrc -o smallworld_res.rcc
qt5_add_resources(UI_RESOURCES src/sworld_res.qrc OPTIONS -compress-algo zstd -compress 19)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

if(GK_CUDA_FFT)
    set(GALAXY_CUDA
        ${GALAXY_CUDA}
        src/spectro_cuda.cu
        src/spectro_cuda.h)

    set_property(SOURCE ${GALAXY_CUDA} PROPERTY SKIP_AUTOUIC ON)
endif()

#
# Compile UIC!
#
set(GALAXY_UI_CPP
    ${GALAXY_UI_CPP}
    src/ui/mainwindow.cpp
    src/ui/dialogsettings.cpp
    src/ui/aboutdialog.cpp
    src/ui/gkaudioplaydialog.cpp
    src/ui/spectrodialog.cpp
    src/ui/gk_vu_meter_widget.cpp
    src/ui/sendreportdialog.cpp
    src/radiolibs.cpp
    src/dek_db.cpp
    src/file_io.cpp
    src/audio_devices.cpp
    src/gk_fft.cpp
    src/spectro_gui.cpp
    src/gk_audio_encoding.cpp
    src/gk_audio_decoding.cpp
    src/gk_cli.cpp
    src/gk_modem.cpp
    src/gk_frequency_list.cpp
    src/gk_string_funcs.cpp
    src/gk_speech_to_text.cpp
    src/gk_logger.cpp
    src/gk_system.cpp
    src/ui/widgets/gk_submit_msg.cpp
    src/ui/widgets/gk_display_image.cpp
    src/models/tableview/gk_frequency_model.cpp
    src/models/tableview/gk_logger_model.cpp)

if(WIN32 OR MSYS OR MINGW)
    # Nothing here at the moment!
elseif(LINUX)
    # Nothing here at the moment!
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

#
# Do not compile UIC for these!
#
set(GALAXY_SRC_CPP
    ${GALAXY_SRC_CPP}
    src/pa_sinewave.cpp
    src/contrib/udev/device.cpp
    src/contrib/udev/enumerate.cpp
    src/contrib/udev/monitor.cpp
    src/contrib/udev/udev.cpp
    src/contrib/udev/posix/error.cpp
    src/contrib/udev/posix/resource.cpp)

if(LINUX)
    set(GALAXY_SRC_CPP
        ${GALAXY_SRC_CPP})
elseif(WIN32 OR MSYS OR MINGW)
    set(GALAXY_SRC_CPP
        ${GALAXY_SRC_CPP})
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

set_property(SOURCE ${GALAXY_SRC_CPP} PROPERTY SKIP_AUTOUIC ON)

#
# Do not compile under MinGW for these sources!
#
set(GALAXY_SRC_CPP_NON_MINGW
    ${GALAXY_SRC_CPP_NON_MINGW}) # Setup a dummy variable for if this variable is otherwise not needed (i.e. we're not compiling under MinGW!)....

# Setup the real MinGW variables for if they're actually needed (i.e. we're actually compiling under MinGW)!
if (!MSYS OR !MINGW)
    set(GALAXY_SRC_CPP_NON_MINGW
        ${GALAXY_SRC_CPP_NON_MINGW}
        src/contrib/getopt.c)
endif()

set_property(SOURCE ${GALAXY_SRC_CPP_NON_MINGW} PROPERTY SKIP_AUTOUIC ON)

#
# Compile UIC!
#
set(GALAXY_OBJ_HEADERS
    ${GALAXY_OBJ_HEADERS}
    src/defines.hpp
    src/ui/mainwindow.hpp
    src/ui/dialogsettings.hpp
    src/ui/aboutdialog.hpp
    src/ui/gkaudioplaydialog.hpp
    src/ui/spectrodialog.hpp
    src/ui/gk_vu_meter_widget.hpp
    src/ui/sendreportdialog.hpp
    src/radiolibs.hpp
    src/dek_db.hpp
    src/file_io.hpp
    src/audio_devices.hpp
    src/gk_fft.hpp
    src/spectro_gui.hpp
    src/gk_audio_encoding.hpp
    src/gk_audio_decoding.hpp
    src/gk_cli.hpp
    src/gk_modem.hpp
    src/gk_frequency_list.hpp
    src/gk_string_funcs.hpp
    src/gk_speech_to_text.hpp
    src/gk_logger.hpp
    src/gk_system.hpp
    src/ui/widgets/gk_submit_msg.hpp
    src/ui/widgets/gk_display_image.hpp
    src/models/tableview/gk_frequency_model.hpp
    src/models/tableview/gk_logger_model.hpp)

if(WIN32 OR MSYS OR MINGW)
    # Nothing here at the moment!
elseif(LINUX)
    # Nothing here at the moment!
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

#
# Do not compile UIC for these!
#
set(GALAXY_SRC_HEADERS
    ${GALAXY_SRC_HEADERS}
    src/pa_audio_buf.hpp
    src/pa_sinewave.hpp
    src/gk_timer.hpp
    src/gk_circ_buffer.hpp
    src/contrib/rapidcsv/src/rapidcsv.h
    src/contrib/AudioFile/AudioFile.h
    src/contrib/udev/device.hpp
    src/contrib/udev/enumerate.hpp
    src/contrib/udev/monitor.hpp
    src/contrib/udev/udev.hpp
    src/contrib/udev/posix/error.hpp
    src/contrib/udev/posix/resource.hpp)

if(LINUX)
    set(GALAXY_SRC_HEADERS
        ${GALAXY_SRC_HEADERS})
elseif(WIN32 OR MSYS OR MINGW)
    set(GALAXY_SRC_HEADERS
        ${GALAXY_SRC_HEADERS})
elseif(APPLE)
    message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
endif()

set_property(SOURCE ${GALAXY_SRC_HEADERS} PROPERTY SKIP_AUTOUIC ON)

#
# Do not compile under MinGW for these sources!
#
set(GALAXY_SRC_HEADERS_NON_MINGW
    ${GALAXY_SRC_HEADERS_NON_MINGW}) # Setup a dummy variable for if this variable is otherwise not needed (i.e. we're not compiling under MinGW!)....

# Setup the real MinGW variables for if they're actually needed (i.e. we're actually compiling under MinGW)!
if (!MSYS OR !MINGW)
    set(GALAXY_SRC_HEADERS_NON_MINGW
        ${GALAXY_SRC_HEADERS_NON_MINGW}
        src/contrib/getopt.h)
endif()

set_property(SOURCE ${GALAXY_SRC_HEADERS_NON_MINGW} PROPERTY SKIP_AUTOUIC ON)

if (MSYS OR MINGW)
    set(GK_EXTRA_LIBS
        ${GK_EXTRA_LIBS}
        -lhid
        -lsetupapi
        -lole32
        -loleaut32)
else()
    #
    # Just an empty container for now!
    #
    set(GK_EXTRA_LIBS
        ${GK_EXTRA_LIBS})
endif()

add_definitions("-DQWT_DLL")

if (GFYRE_SWORLD_DBG_VERBOSITY)
    message(STATUS "Enabled maximum verbosity with Small World Deluxe and its use of third-party libraries throughout the application itself; debug mode now activated!")
    add_definitions(-DGFYRE_SWORLD_DBG_VERBOSITY)
endif()

if (GFYRE_PORTAUDIO_DBG_VERBOSITY)
    message(STATUS "Enabled maximum verbosity with the PortAudio library throughout Small World Deluxe; debug mode now activated!")
    add_definitions(-DGFYRE_PORTAUDIO_DBG_VERBOSITY_ENBL)
endif()

if (ENBL_VALGRIND_SUPPORT)
    if (LINUX)
        add_definitions(-DGK_ENBL_VALGRIND_SUPPORT)
    endif()
endif()

#
# Build Small World Deluxe as a static package!
#
if(GFYRE_BUILD_STATIC)
    add_library(galaxy STATIC ${GALAXY_SRC_CPP} ${GALAXY_SRC_CPP_NON_MINGW} ${GALAXY_UI_CPP} ${UI_MOC} ${GALAXY_OBJ_HEADERS} ${GALAXY_SRC_HEADERS} ${GALAXY_SRC_HEADERS_NON_MINGW} ${UI_HEADERS} ${GALAXY_PORTAUDIO_CPP} ${GALAXY_CODEC2_CPP} ${GALAXY_CODEC2_HPP} ${GALAXY_WSJTX_QRA64_C} ${GALAXY_WSJTX_WSPR_C})
    if(GK_CUDA_FFT)
        add_library(galaxy_cuda STATIC ${GALAXY_CUDA})
    endif()
    
    # add_library(galaxy_fort STATIC ${GALAXY_WSJTX_QRA64_FORT})
else()
    set(GFYRE_BUILD_SHARED ON)
endif(GFYRE_BUILD_STATIC)

#
# Build Small World Deluxe as a shared package!
#
if(GFYRE_BUILD_SHARED)
    add_library(galaxy SHARED ${GALAXY_SRC_CPP} ${GALAXY_SRC_CPP_NON_MINGW} ${GALAXY_UI_CPP} ${UI_MOC} ${GALAXY_OBJ_HEADERS} ${GALAXY_SRC_HEADERS} ${GALAXY_SRC_HEADERS_NON_MINGW} ${UI_HEADERS} ${GALAXY_PORTAUDIO_CPP} ${GALAXY_CODEC2_CPP} ${GALAXY_CODEC2_HPP} ${GALAXY_WSJTX_QRA64_C} ${GALAXY_WSJTX_WSPR_C})
    if(GK_CUDA_FFT)
        add_library(galaxy_cuda STATIC ${GALAXY_CUDA})
    endif()
    
    # add_library(galaxy_fort STATIC ${GALAXY_WSJTX_QRA64_FORT})
endif(GFYRE_BUILD_SHARED)

set_property(TARGET galaxy PROPERTY AUTOMOC ON)
target_link_libraries(galaxy PUBLIC ${GK_EXTRA_LIBS} Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Network Qt5::SerialPort Qt5::PrintSupport Qt5::TextToSpeech Qt5::Test Threads::Threads SingleApplication::SingleApplication sentry::sentry ${LIBS})

qt5_add_binary_resources(resources src/sworld_res.qrc OPTIONS -no-compress)
add_dependencies(galaxy resources)

set(SWRLD_EXEC ${SWRLD_EXEC}
    src/main.cpp)

add_executable(smallworld WIN32 ${SWRLD_EXEC})
target_compile_options(smallworld PRIVATE -fPIC)
target_link_libraries(smallworld PRIVATE galaxy)

if(GK_CUDA_FFT)
    target_link_libraries(smallworld PRIVATE galaxy_cuda)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if (NOT LINUX)
    #
    # Copy the crashpad handler to the root of the binary build directory, for easy access!
    # https://cmake.org/cmake/help/latest/module/FetchContent.html
    # https://www.thetopsites.net/article/52110824.shtml
    #
    if(WIN32 OR MSYS OR MINGW)
        add_custom_command(TARGET smallworld
                        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_CURRENT_BINARY_DIR}/src/contrib/sentry-native/crashpad_build/handler/crashpad_handler.exe
                        ${CMAKE_CURRENT_BINARY_DIR}/crashpad_handler.exe)
    else()
        add_custom_command(TARGET smallworld
                        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_CURRENT_BINARY_DIR}/src/contrib/sentry-native/crashpad_build/handler/crashpad_handler
                        ${CMAKE_CURRENT_BINARY_DIR}/crashpad_handler)
    endif()
endif()

INSTALL(TARGETS smallworld)

#
# https://stackoverflow.com/questions/23995019/what-is-the-modern-method-for-setting-general-compile-flags-in-cmake
# https://stackoverflow.com/questions/20105167/c-error-failure-localefacet-s-create-c-locale-name-not-valid-when-runni
#
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Using regular Clang or AppleClang
    add_compile_options(-Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context -enable-clocale)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Using GCC
    add_compile_options(-Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context -enable-clocale)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # Using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Using Microsoft Visual Studio C++
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W3 /WX)
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W3 /WX /GL)
    endif()
endif()

if (GFYRE_ENABLE_PVS_STUDIO)
    include(PVS-Studio.cmake)
    pvs_studio_add_target(TARGET smallworld.analyze ALL
                          PREPROCESSOR clang
                          OUTPUT FORMAT errorfile
                          ANALYZE smallworld galaxy
                          LOG swd.err)
    message(STATUS "Enabled static analysis with PVS-Studio!")
else()
    message(STATUS "Disabled static analysis with PVS-Studio!")
endif()
