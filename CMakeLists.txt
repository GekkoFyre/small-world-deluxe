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
#   Copyright (C) 2020 - 2021. GekkoFyre.
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
#   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
#

cmake_minimum_required(VERSION 3.5 FATAL_ERROR) # This version is needed to properly search for libpthreads

cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0025 NEW)

#
# https://cmake.org/cmake/help/latest/policy/CMP0054.html
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.1.0")
	cmake_policy(SET CMP0054 NEW)
endif()

#
# https://cmake.org/cmake/help/latest/policy/CMP0074.html
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
	cmake_policy(SET CMP0074 NEW)
endif()

#
# https://cmake.org/cmake/help/git-stage/policy/CMP0087.html
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14.0")
	cmake_policy(SET CMP0087 NEW)
endif()

#
# https://cmake.org/cmake/help/git-stage/policy/CMP0091.html
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15.0")
    cmake_policy(SET CMP0091 NEW)
endif()

#
# RPATH is aimed at Linux installations primarily
#
set(CMAKE_INSTALL_RPATH $ORIGIN $ORIGIN/lib) # Relative path to lib-dir...
set(INSTALL_RPATH_USE_LINK_PATH true)

project(smallworld LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#
# Auto everything!
# Reference: https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
# https://doc.qt.io/qt-5/cmake-manual.html
#
# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)
# set(CMAKE_AUTOUIC ON)

# NOTE: Do not change this unless you know what you are doing!
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")

#
# Custom options specific to Small World Deluxe and/or GekkoFyre Networks <https://gekkofyre.io/>
#
option(GFYRE_PACKAGE_MAINTAINER "Are you a package maintainer? If so, set this option to `ON`! It'll enable specific build settings to make your life easier." ON)
option(GFYRE_BUILD_STATIC "Build Small World Deluxe as a static library/package. Only available under Microsoft Windows systems." ON)
option(GFYRE_BUILD_SHARED "Build Small World Deluxe as a shared library/package. This will result in a smaller executable at the cost of requiring further DLL dependencies." OFF)
option(GFYRE_SWORLD_DBG_VERBOSITY "Enable DEBUG verbosity for Small World Deluxe and its use of third-party libraries throughout the program itself." OFF)
option(GFYRE_BOOST_CPP_DBG "Enable the use of debug libraries for the Boost C++ dependency!" OFF)

option(GFYRE_ENBL_VALGRIND_SUPPORT "Allows for the use of Valgrind memory analysis on the application, thereby disabling certain, otherwise needed features that enhance the user-experience." OFF)
option(GFYRE_ENBL_CLANG_ADDRESS_SANITIZER "LLVM's AddressSanitizer is a fast memory error detector. Clang support must be enabled for this to work." OFF)

option(BUILD_CODEC2_SUPPORT "Enable support for the 'Codec2' open source speech codec library." OFF)

#
# Install / copy directory for CMake modules...
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/modules")

#
# Detect if we are dealing with the Linux operating system and not Apple OS/X
#
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

option(GFYRE_ENABLE_PVS_STUDIO "Enable the static analysis of the Small World Deluxe project via PVS-Studio. More details can be found here: https://github.com/viva64/pvs-studio-cmake-examples" OFF)

if(MSYS OR MINGW)
    add_definitions(-DCMAKE_CROSSCOMPILING=1)
    add_definitions(-DRUN_HAVE_STD_REGEX=0)
    add_definitions(-DRUN_HAVE_POSIX_REGEX=0)
endif()

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

#
# Nuspell
# https://nuspell.github.io/
#
if (WIN32 OR MSYS OR MINGW)
	find_package(Nuspell REQUIRED)
	if (NUSPELL_FOUND)
		include_directories(${NUSPELL_INCLUDE_DIRS})
		set(LIBS ${LIBS} ${NUSPELL_LIBRARIES})
		message(STATUS "The 'Nuspell' spelling and grammar checking libraries have been configured!")
	else()
		message(FATAL_ERROR "The 'Nuspell' spelling and grammar checking libraries could not be found!")
	endif(NUSPELL_FOUND)
endif()

find_package(QWT REQUIRED)
if (QWT_FOUND)
    include_directories(${QWT_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${QWT_LIBRARIES})
    add_definitions(-DQWT_DLL)
else()
    message(FATAL_ERROR "The 'QWT' graphing/charting library could not be found!")
endif(QWT_FOUND)

if (LINUX)
    set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Use static libraries when linking Boost C++.")
elseif(WIN32 OR MSYS OR MINGW)
    set(Boost_USE_STATIC_LIBS ON CACHE BOOL "Use static libraries when linking Boost C++.")
else()
    message(SEND_ERROR "The only operating systems and/or build environments we OFFICIALLY support as of the moment are Microsoft Windows, MinGW/MSYS, and Linux. Sorry!")
endif()

if (GFYRE_BOOST_CPP_DBG)
    set(Boost_USE_DEBUG_LIBS ON CACHE BOOL "Enable the use of debug libraries for the Boost C++ dependency!")
endif()

set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multithreaded libraries when linking Boost C++.")
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime when linking Boost C++.")
find_package(Boost 1.64.0 REQUIRED COMPONENTS "filesystem"
                                              "system"
                                              "exception"
                                              "locale")
                                              
if(WIN32 OR MSYS OR MINGW)
    if(${GFYRE_PACKAGE_MAINTAINER} EQUAL ON)
        message(STATUS "Using package maintainer settings.")
        if (MSYS OR MINGW OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            # http://stackoverflow.com/questions/9742003/platform-detection-in-cmake
            add_definitions(-D_WIN32_WINNT=NTDDI_VISTASP1)
        endif()
    endif()
endif()

if(GFYRE_BUILD_STATIC)
    add_definitions(-DBUILD_SHARED=0)
endif()

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})
    add_definitions(-DBOOST_ALL_NO_LIB)
    message(STATUS "Boost C++ libraries have been found.")
else()
    message(FATAL_ERROR "The required 'Boost C++' libraries are either not installed, or not multithreaded and/or of not an up-to-date version. Boost C++ 1.54.0 is the minimally required version.")
endif(Boost_FOUND)

#
# QXmpp
# https://github.com/qxmpp-project/qxmpp
#
function(install_qxmpp)
    find_package(QXmpp REQUIRED)
    if (QXMPP_FOUND)
        include_directories(${QXMPP_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${QXMPP_LIBRARIES})
        include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/QXmpp.cmake")
        message(STATUS "The 'QXmpp' Extensible Messaging and Presence Protocol (XMPP) libraries have been configured!")
    else()
        message(FATAL_ERROR "The 'QXmpp' Extensible Messaging and Presence Protocol (XMPP) libraries could not be found!")
    endif(QXMPP_FOUND)
endfunction()

#
# Hamlib
# https://hamlib.github.io/
#
find_package(Hamlib REQUIRED)
if (HAMLIB_FOUND)
    include_directories(${HAMLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${HAMLIB_LIBRARIES})
    ADD_DEFINITIONS(-DUSE_HAMLIB=1)
else()
    message(FATAL_ERROR "The 'Hamlib' amateur radio library could not be found!")
endif(HAMLIB_FOUND)

set(GALAXY_HAMLIB_CPP
    ${GALAXY_HAMLIB_CPP}
    src/contrib/hamlib++/rigclass.cc
    src/contrib/hamlib++/rotclass.cc
    src/contrib/hamlib++/include/hamlib/rigclass.h
    src/contrib/hamlib++/include/hamlib/rotclass.h)

#
# OpusEnc
# https://gitlab.xiph.org/xiph/libopusenc
#
find_package(OpusEnc REQUIRED)
if (OpusEnc_FOUND)
    include_directories(${OpusEnc_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${OpusEnc_LIBRARIES})
    message(STATUS "The 'libopusenc' audio (de-/en-)coding file creation libraries have been configured!")
else()
    message(FATAL_ERROR "The 'libopusenc' audio (de-/en-)coding file creation libraries could not be found!")
endif(OpusEnc_FOUND)

#
# OpenAL
# https://www.openal.org/
#
find_package(OpenAL REQUIRED)
if (OPENAL_FOUND)
    include_directories(${OPENAL_INCLUDE_DIR})
    set(LIBS ${LIBS} ${OPENAL_LIBRARY})
    message(STATUS "The open source 'OpenAL' 3D audio library has been configured!")
else()
    message(FATAL_ERROR "The open source 'OpenAL' 3D audio library could not be found!")
endif(OPENAL_FOUND)

#
# libsndfile
# https://github.com/libsndfile/libsndfile
#
find_package(Sndfile REQUIRED)
if (SNDFILE_FOUND)
    include_directories(${SNDFILE_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${SNDFILE_LIBRARIES})
    message(STATUS "The 'libsndfile' audio (de-/en-)coding libraries have been configured!")
else()
    message(FATAL_ERROR "The 'libsndfile' audio (de-/en-)coding libraries could not be found!")
endif(SNDFILE_FOUND)

install_qxmpp() # Install QXmpp as non-static!

if (LINUX)
    find_package(UDev REQUIRED)
    if (UDEV_FOUND)
        include_directories(${UDEV_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${UDEV_LIBRARIES})
        message(STATUS "The 'udev' SystemD libraries have been configured!")
    else()
        message(FATAL_ERROR "The 'udev' SystemD libraries could not be found!")
    endif(UDEV_FOUND)
endif(LINUX)

if(WIN32 OR MSYS OR MINGW)
    if (GFYRE_BUILD_STATIC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
        option(BUILD_SHARED_LIBS "Set this option to OFF in order to compile a static set of libraries." OFF)
    else()
        option(BUILD_SHARED_LIBS "Set this option to OFF in order to compile a static set of libraries." ON)
    endif(GFYRE_BUILD_STATIC)
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
    message(STATUS "The 'Threads' libraries have been configured!")
else()
    message(FATAL_ERROR "Could not find the Threads library!")
endif(Threads_FOUND)

#
# ICU
#
find_package(ICU REQUIRED COMPONENTS i18n data uc)
if (ICU_FOUND)
    set(LIBS ${LIBS} ${ICU_LIBRARIES})
    message(STATUS "The 'ICU' set of internationalization libraries has been configured!")
else()
    message(FATAL_ERROR "The 'ICU' set of internationalization libraries culd not be found!")
endif(ICU_FOUND)

#
# Aria2 by Tatsuhiro Tsujikawa and Ross Smith II et al.
# https://aria2.github.io/
#
find_package(Aria2 REQUIRED)
if (ARIA2_FOUND)
    include_directories(${ARIA2_INCLUDE_DIRS})
    # set(LIBS ${LIBS} ${ARIA2_LIBRARIES})
	message(STATUS "The 'aria2' command-line tool and library for transferring network data has been configured!")
else()
	message(FATAL_ERROR "The 'aria2' command-line tool and library for transferring network data could not be found!")
endif(ARIA2_FOUND)

find_package(LibXml2 REQUIRED)
if (LibXml2_FOUND)
    include_directories(${LIBXML2_INCLUDE_DIRS})
    # set(LIBS ${LIBS} ${LIBXML2_LIBRARIES})
	message(STATUS "The 'LibXml2' library has been configured!")
else()
	message(FATAL_ERROR "The 'LibXml2' library could not be found!")
endif(LibXml2_FOUND)

find_package(Gettext REQUIRED)
if (GETTEXT_FOUND)
    # set(LIBS ${LIBS} ${GETTEXT_LIBRARIES})
	message(STATUS "The 'gettext' library has been configured!")
else()
	message(FATAL_ERROR "The 'gettext' library could not be found!")
endif(GETTEXT_FOUND)

if(WIN32 OR MSYS OR MINGW)
	find_package(LibIntl REQUIRED)
	if (LIBINTL_FOUND)
		# set(LIBS ${LIBS} ${LIBINTL_LIBRARIES})
		message(STATUS "The 'libintl' internationalization library has been configured!")
	else()
		message(FATAL_ERROR "The 'libintl' internationalization library could not be found!")
	endif(LIBINTL_FOUND)
endif()

find_package(LibSsh2 REQUIRED)
if (LIBSSH2_FOUND)
    # set(LIBS ${LIBS} ${LIBSSH2_LIBRARIES})
	message(STATUS "The 'libssh2' SSH libraries have been configured!")
else()
	message(FATAL_ERROR "The 'libssh2' SSH libraries could not be found!")
endif(LIBSSH2_FOUND)

find_package(SQLite3 REQUIRED)
if (SQLite3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
    # set(LIBS ${LIBS} ${SQLite3_LIBRARIES})
	message(STATUS "The 'SQLite3' library has been configured!")
else()
	message(FATAL_ERROR "The 'SQLite3' library could not be found!")
endif(SQLite3_FOUND)

#
# Nuspell
# https://nuspell.github.io/
#
if (LINUX OR !MSYS OR !MINGW)
	find_package(Nuspell REQUIRED)
	if (NUSPELL_FOUND)
		include_directories(${NUSPELL_INCLUDE_DIRS})
		set(LIBS ${LIBS} ${NUSPELL_LIBRARIES})
		message(STATUS "The 'Nuspell' spelling and grammar checking libraries have been configured!")
	else()
		message(FATAL_ERROR "The 'Nuspell' spelling and grammar checking libraries could not be found!")
	endif(NUSPELL_FOUND)
endif()

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    set(LIBS ${LIBS} ${OPENSSL_SSL_LIBRARIES})
    message(STATUS "The 'OpenSSL' cryptography libraries have been configured!")
else()
    message(FATAL_ERROR "The 'OpenSSL' cryptography libraries could not be found!")
endif(OPENSSL_FOUND)

find_package(Qt5 COMPONENTS Core Gui Widgets Network LinguistTools Multimedia SerialPort PrintSupport Xml TextToSpeech Test REQUIRED)

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${ZLIB_LIBRARIES})
    message(STATUS "The 'ZLIB' compression library has been configured!")
else()
    message(FATAL_ERROR "The 'ZLIB' compression library could not be found!")
endif(ZLIB_FOUND)

set_property(SOURCE ${GALAXY_HAMLIB_CPP} PROPERTY SKIP_AUTOUIC ON)

if (BUILD_CODEC2_SUPPORT)
    if (MSYS OR MINGW OR LINUX OR UNIX)
        message(STATUS "Support for 'Codec2' has been enabled! Finding package...")
        find_package(Codec2 REQUIRED)
        if (CODEC2_FOUND)
            include_directories(${CODEC2_INCLUDE_DIRS})
            set(LIBS ${LIBS} ${CODEC2_LIBRARIES})
            add_definitions(-DCODEC2_LIBS_ENBLD)
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
else()
    message(FATAL_ERROR "Either `libusb.h` and/or `usb.h` could not be found for the 'libusb' system device and enumeration libraries!")
endif(USB_FOUND)

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
set(SENTRY_INTEGRATION_QT ON CACHE BOOL "Build Qt integration." FORCE)
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" FORCE)
set(SENTRY_PIC ON CACHE BOOL "Build Sentry as a position independent library" FORCE)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(SENTRY_BUILD_RUNTIMESTATIC OFF CACHE BOOL "Disables linking with the static MSVC runtime. Has no effect if the compiler is not MSVC.")
endif()

#
# Use Crashpad (and therefore the more modern crash reporting system) for all major operating systems based around the personal computer (Android, iOS, etc. and the like are therefore unsupported but we currently have no plans for the mobile market right now)...
# A feature matrix can be seen here towards the bottom of the page: https://github.com/getsentry/sentry-native
#
set(SENTRY_BACKEND "crashpad" CACHE STRING "The type of crash manager to use." FORCE)
set(SENTRY_CRASHPAD_SYSTEM OFF CACHE BOOL "This instructs the build system to use system-installed breakpad or crashpad libraries instead of using the in-tree version. This is generally not recommended for crashpad, as sentry uses a patched version that has attachment support.")
set(SENTRY_BREAKPAD_SYSTEM OFF CACHE BOOL "This instructs the build system to use system-installed breakpad or crashpad libraries instead of using the in-tree version. This is generally not recommended for crashpad, as sentry uses a patched version that has attachment support.")

set(SENTRY_BUILD_TESTS OFF CACHE BOOL "Build sentry-native tests. This is not required for the compilation of Small World Deluxe." FORCE)
set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "Build sentry-native example(s). This is not required for the compilation of Small World Deluxe." FORCE)
set(SENTRY_LINK_PTHREAD ON CACHE BOOL "Links to the pthread library on Unix targets.")
set(SENTRY_BUILD_FORCE32 OFF CACHE BOOL "Otherwise forces cross-compilation from 64-bit host to 32-bit target. Only has an effect on Linux." FORCE)

if(WIN32)
    set(SENTRY_TRANSPORT "winhttp" CACHE STRING "Please do not modify this." FORCE)
else()
    set(SENTRY_TRANSPORT "curl" CACHE STRING "Please do not modify this." FORCE)
endif()

#
# Google LevelDB [ https://github.com/google/leveldb ]
#
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "Build Google LevelDB tests. This is not required for the compilation of Small World Deluxe." FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "Build Google LevelDB benchmarks. This is not required for the compilation of Small World Deluxe." FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/leveldb")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/leveldb/include")
message(STATUS "The 'Google LevelDB' database libraries have been configured!")

#
# Sentry (native)
#
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/sentry-native")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/sentry-native/include")
message(STATUS "The 'Sentry' error reporting libraries have been configured!")

#
# Opus
# https://opus-codec.org/development/
#
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/opus")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/opus/include")
message(STATUS "The 'Opus' audio (de-/en-)coding libraries have been configured!")

#
# KissFFT by Mark Borgerding
# https://github.com/mborgerding/kissfft
#
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/kissfft")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/kissfft")
message(STATUS "The 'KissFFT' fourier transform libraries have been configured!")

#

#
# AudioFile by Adam Stark
# https://github.com/adamstark/AudioFile
#
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/AudioFile")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/AudioFile")
message(STATUS "The 'AudioFile' libraries have been configured!")

#
# c-ares by Daniel Stenberg et al.
# https://c-ares.haxx.se/
#
set(CARES_STATIC ON CACHE BOOL "Build c-ares as a static library." FORCE)
set(CARES_SHARED OFF CACHE BOOL "Build c-ares as a shared library." FORCE)
if (CARES_STATIC)
	set(CARES_STATIC_PIC ON CACHE BOOL "Build the c-ares static library as PIC (position independent)." FORCE)
endif()

set(CARES_BUILD_TESTS OFF CACHE BOOL "Build and run tests for the c-ares set of libraries." FORCE)
set(CARES_BUILD_TOOLS OFF CACHE BOOL "Build tools for the c-ares set of libraries." FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/c-ares")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/contrib/c-ares/include")
message(STATUS "The 'c-ares' library for asynchronous DNS requests has been configured!")

#
# WSJT-X
#
set(GALAXY_WSJTX_QRA64_C
    ${GALAXY_WSJTX_QRA64_C}
    src/contrib/wsjt-x/lib/ftrsd/int.h
    src/contrib/wsjt-x/lib/qra/qra64/main.c
    src/contrib/wsjt-x/lib/qra/qra64/fadengauss.c
    src/contrib/wsjt-x/lib/qra/qra64/fadenlorentz.c
    src/contrib/wsjt-x/lib/qra/qra64/qra64_subs.c
    src/contrib/wsjt-x/lib/qra/qra64/qra64.c
    src/contrib/wsjt-x/lib/qra/qra64/qra64.h
    src/contrib/wsjt-x/lib/qra/qracodes/normrnd.c
    src/contrib/wsjt-x/lib/qra/qracodes/normrnd.h
    src/contrib/wsjt-x/lib/qra/qracodes/npfwht.c
    src/contrib/wsjt-x/lib/qra/qracodes/npfwht.h
    src/contrib/wsjt-x/lib/qra/qracodes/pdmath.c
    src/contrib/wsjt-x/lib/qra/qracodes/pdmath.h
    src/contrib/wsjt-x/lib/qra/qracodes/qra12_63_64_irr_b.c
    src/contrib/wsjt-x/lib/qra/qracodes/qra12_63_64_irr_b.h
    src/contrib/wsjt-x/lib/qra/qracodes/qra13_64_64_irr_e.c
    src/contrib/wsjt-x/lib/qra/qracodes/qra13_64_64_irr_e.h
    src/contrib/wsjt-x/lib/qra/qracodes/qracodes.c
    src/contrib/wsjt-x/lib/qra/qracodes/qracodes.h)

set_property(SOURCE ${GALAXY_WSJTX_QRA64_C} PROPERTY SKIP_AUTOUIC ON)

set(GALAXY_WSJTX_WSPR_C
    ${GALAXY_WSJTX_WSPR_C}
    src/contrib/wsjt-x/lib/wsprd/nhash.c
    src/contrib/wsjt-x/lib/wsprd/wsprd_utils.c
    src/contrib/wsjt-x/lib/wsprd/wsprd_utils.h
    src/contrib/wsjt-x/lib/wsprd/fano.c
    src/contrib/wsjt-x/lib/wsprd/fano.h
    src/contrib/wsjt-x/lib/wsprd/jelinek.c
    src/contrib/wsjt-x/lib/wsprd/jelinek.h)

set_property(SOURCE ${GALAXY_WSJTX_WSPR_C} PROPERTY SKIP_AUTOUIC ON)

set(GALAXY_GIST_CPP
    ${GALAXY_GIST_CPP}
    src/contrib/Gist/src/AccelerateFFT.cpp
    src/contrib/Gist/src/CoreFrequencyDomainFeatures.cpp
    src/contrib/Gist/src/CoreTimeDomainFeatures.cpp
    src/contrib/Gist/src/Gist.cpp
    src/contrib/Gist/src/MFCC.cpp
    src/contrib/Gist/src/OnsetDetectionFunction.cpp
    src/contrib/Gist/src/WindowFunctions.cpp
    src/contrib/Gist/src/Yin.cpp)

set_property(SOURCE ${GALAXY_GIST_CPP} PROPERTY SKIP_AUTOUIC ON)

set(GALAXY_GIST_HPP
    ${GALAXY_GIST_HPP}
    src/contrib/Gist/src/Yin.h
    src/contrib/Gist/src/WindowFunctions.h
    src/contrib/Gist/src/OnsetDetectionFunction.h
    src/contrib/Gist/src/MFCC.h
    src/contrib/Gist/src/Gist.h
    src/contrib/Gist/src/CoreTimeDomainFeatures.h
    src/contrib/Gist/src/CoreFrequencyDomainFeatures.h
    src/contrib/Gist/src/AccelerateFFT.h)

set_property(SOURCE ${GALAXY_GIST_HPP} PROPERTY SKIP_AUTOUIC ON)

if(WIN32 AND !MSYS AND !MINGW)
    add_definitions(-DGFYRE_ENBL_MSVC_WINTOAST)
    set(GALAXY_WINTOAST_HPP
        ${GALAXY_WINTOAST_HPP}
        src/contrib/WinToast/src/wintoastlib.h)

    set_property(SOURCE ${GALAXY_WINTOAST_HPP} PROPERTY SKIP_AUTOUIC ON)

    set(GALAXY_WINTOAST_CPP
        ${GALAXY_WINTOAST_CPP}
        src/contrib/WinToast/src/wintoastlib.cpp)

    set_property(SOURCE ${GALAXY_WINTOAST_CPP} PROPERTY SKIP_AUTOUIC ON)
else()
    #
    # Deliberately left empty!
    #
    set(GALAXY_WINTOAST_HPP
        ${GALAXY_WINTOAST_HPP})

    set_property(SOURCE ${GALAXY_WINTOAST_HPP} PROPERTY SKIP_AUTOUIC ON)

    set(GALAXY_WINTOAST_CPP
        ${GALAXY_WINTOAST_CPP})

    set_property(SOURCE ${GALAXY_WINTOAST_CPP} PROPERTY SKIP_AUTOUIC ON)
endif()

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
    src/ui/sendreportdialog.ui
    src/ui/widgets/gk_vu_change_widget.ui
    src/ui/xmpp/gkxmpprosterdialog.ui
    src/ui/xmpp/gkxmppmessagedialog.ui
    src/ui/xmpp/gkxmppregistrationdialog.ui)

# https://doc.qt.io/qt-5/cmake-command-reference.html
# rcc -no-compress -binary smallworld_res.qrc -o smallworld_res.rcc
qt5_add_resources(UI_RESOURCES src/sworld_res.qrc OPTIONS -compress-algo zstd -compress 19)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

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
    src/ui/sendreportdialog.cpp
    src/ui/xmpp/gkxmpprosterdialog.cpp
    src/ui/xmpp/gkxmppmessagedialog.cpp
    src/ui/xmpp/gkxmppregistrationdialog.cpp
    src/radiolibs.cpp
    src/dek_db.cpp
    src/file_io.cpp
    src/audio_devices.cpp
    src/gk_fyr_data.cpp
    src/gk_fft_audio.cpp
    src/gk_waterfall_gui.cpp
    src/gk_spectro_color_maps.cpp
    src/gk_audio_encoding.cpp
    src/gk_audio_decoding.cpp
    src/gk_xmpp_client.cpp
	src/gk_xmpp_msg_handler.cpp
    src/gk_cli.cpp
    src/gk_modem.cpp
    src/gk_frequency_list.cpp
    src/gk_string_funcs.cpp
    src/gk_text_to_speech.cpp
    src/gk_logger.cpp
    src/gk_system.cpp
    src/gk_pcm_file_stream.cpp
    src/pa_stream_handler.cpp
    src/gk_sinewave.cpp
	src/gk_exception.cpp
    src/pa_audio_player.cpp
    src/ui/widgets/gk_vu_change_widget.cpp
    src/ui/widgets/gk_vu_meter_widget.cpp
    src/ui/widgets/gk_submit_msg.cpp
    src/ui/widgets/gk_display_image.cpp
	src/models/splash/gk_splash_disp_model.cpp
    src/models/system/gk_network_ping_model.cpp
    src/models/tableview/gk_frequency_model.cpp
    src/models/tableview/gk_logger_model.cpp
    src/models/tableview/gk_active_msgs_model.cpp
    src/models/tableview/gk_callsign_msgs_model.cpp
    src/models/tableview/gk_xmpp_recv_msgs_model.cpp
	src/models/tableview/gk_xmpp_roster_presence_model.cpp
	src/models/tableview/gk_qdatetime_filter_proxy_model.cpp
	src/models/tableview/gk_xmpp_roster_pending_model.cpp
	src/models/tableview/gk_xmpp_roster_blocked_model.cpp
    src/models/treeview/xmpp/gk_xmpp_roster_model.cpp)

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
    ${GALAXY_SRC_CPP})

if(LINUX)
    set(GALAXY_SRC_CPP
        ${GALAXY_SRC_CPP}
        src/contrib/udev/device.cpp
        src/contrib/udev/enumerate.cpp
        src/contrib/udev/monitor.cpp
        src/contrib/udev/udev.cpp
        src/contrib/udev/posix/error.cpp
        src/contrib/udev/posix/resource.cpp)
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
    src/ui/sendreportdialog.hpp
    src/ui/xmpp/gkxmpprosterdialog.hpp
    src/ui/xmpp/gkxmppmessagedialog.hpp
    src/ui/xmpp/gkxmppregistrationdialog.hpp
    src/radiolibs.hpp
    src/dek_db.hpp
    src/file_io.hpp
    src/audio_devices.hpp
    src/gk_fyr_data.hpp
    src/gk_fft_audio.hpp
    src/gk_waterfall_gui.hpp
    src/gk_spectro_color_maps.hpp
    src/gk_audio_encoding.hpp
    src/gk_audio_decoding.hpp
    src/gk_xmpp_client.hpp
	src/gk_xmpp_msg_handler.hpp
    src/gk_cli.hpp
    src/gk_modem.hpp
    src/gk_frequency_list.hpp
    src/gk_string_funcs.hpp
    src/gk_text_to_speech.hpp
    src/gk_logger.hpp
    src/gk_system.hpp
    src/gk_pcm_file_stream.hpp
    src/pa_stream_handler.hpp
    src/gk_sinewave.hpp
	src/gk_exception.hpp
    src/gk_waterfall_data.hpp
    src/pa_audio_player.hpp
    src/ui/widgets/gk_vu_change_widget.hpp
    src/ui/widgets/gk_vu_meter_widget.hpp
    src/ui/widgets/gk_submit_msg.hpp
    src/ui/widgets/gk_display_image.hpp
	src/models/splash/gk_splash_disp_model.hpp
    src/models/system/gk_network_ping_model.hpp
    src/models/tableview/gk_frequency_model.hpp
    src/models/tableview/gk_logger_model.hpp
    src/models/tableview/gk_active_msgs_model.hpp
    src/models/tableview/gk_callsign_msgs_model.hpp
    src/models/tableview/gk_xmpp_recv_msgs_model.hpp
	src/models/tableview/gk_xmpp_roster_presence_model.hpp
	src/models/tableview/gk_qdatetime_filter_proxy_model.hpp
	src/models/tableview/gk_xmpp_roster_pending_model.hpp
	src/models/tableview/gk_xmpp_roster_blocked_model.hpp
    src/models/treeview/xmpp/gk_xmpp_roster_model.hpp)

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
set(GK_APP_VERS_HPP
	${GK_APP_VERS_HPP}
	src/gk_app_vers.hpp)

set_property(SOURCE ${GK_APP_VERS_HPP} PROPERTY SKIP_AUTOUIC ON)

#
# Compile UIC!
#
set(GK_AUTO_UPDATE_CPP
	${GK_AUTO_UPDATE_CPP}
	src/update/main.cpp
	src/update/gk_auto_update.cpp
	src/update/gk_check_update.cpp)

#
# Compile UIC!
#
set(GK_AUTO_UPDATE_HEADERS
	${GK_AUTO_UPDATE_HEADERS}
	src/update/gk_auto_update.hpp
	src/update/gk_check_update.hpp)

#
# Do not compile UIC for these!
#
set(GALAXY_SRC_HEADERS
    ${GALAXY_SRC_HEADERS}
    src/gk_timer.hpp
    src/contrib/rapidcsv/src/rapidcsv.h)

if(LINUX)
    set(GALAXY_SRC_HEADERS
        ${GALAXY_SRC_HEADERS}
        src/contrib/udev/device.hpp
        src/contrib/udev/enumerate.hpp
        src/contrib/udev/monitor.hpp
        src/contrib/udev/udev.hpp
        src/contrib/udev/posix/error.hpp
        src/contrib/udev/posix/resource.hpp)
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
        -loleaut32
        -lgdi32
		-lws2_32
		-lwsock32
		-lcrypto
		-liphlpapi
		-lxml2)
else()
    #
    # Just an empty container for now!
    #
    set(GK_EXTRA_LIBS
        ${GK_EXTRA_LIBS})
endif()

if (GFYRE_SWORLD_DBG_VERBOSITY)
    message(STATUS "Enabled maximum verbosity with Small World Deluxe and its use of third-party libraries throughout the application itself; debug mode now activated!")
    add_definitions(-DGFYRE_SWORLD_DBG_VERBOSITY)
endif()

if (ENBL_VALGRIND_SUPPORT)
    add_definitions(-DGK_ENBL_VALGRIND_SUPPORT)
endif()

#
# Build Small World Deluxe as a static package!
#
if(GFYRE_BUILD_STATIC)
    add_library(galaxy STATIC ${GALAXY_SRC_CPP} ${GALAXY_SRC_CPP_NON_MINGW} ${GALAXY_UI_CPP} ${UI_MOC} ${GALAXY_OBJ_HEADERS} ${GALAXY_SRC_HEADERS} ${GALAXY_SRC_HEADERS_NON_MINGW} ${UI_HEADERS} ${GALAXY_WSJTX_WSPR_C} ${GALAXY_GIST_CPP} ${GALAXY_GIST_HPP} ${GALAXY_HAMLIB_CPP} ${GALAXY_WINTOAST_HPP} ${GALAXY_WINTOAST_CPP} ${GK_APP_VERS_HPP})
    if(WIN32 OR MSYS OR MINGW)
        target_link_libraries(galaxy PUBLIC "-static")
    endif()
else()
    set(GFYRE_BUILD_SHARED ON)
endif(GFYRE_BUILD_STATIC)

#
# Build Small World Deluxe as a shared package!
#
if(GFYRE_BUILD_SHARED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )
    add_library(galaxy SHARED ${GALAXY_SRC_CPP} ${GALAXY_SRC_CPP_NON_MINGW} ${GALAXY_UI_CPP} ${UI_MOC} ${GALAXY_OBJ_HEADERS} ${GALAXY_SRC_HEADERS} ${GALAXY_SRC_HEADERS_NON_MINGW} ${UI_HEADERS} ${GALAXY_WSJTX_WSPR_C} ${GALAXY_GIST_CPP} ${GALAXY_GIST_HPP} ${GALAXY_HAMLIB_CPP} ${GALAXY_WINTOAST_HPP} ${GALAXY_WINTOAST_CPP} ${GK_APP_VERS_HPP})
    
    # add_library(galaxy_fort STATIC ${GALAXY_WSJTX_QRA64_FORT})
endif(GFYRE_BUILD_SHARED)

set_property(TARGET galaxy PROPERTY AUTOMOC ON)
target_link_libraries(galaxy PUBLIC ${GK_EXTRA_LIBS} ICU::i18n ICU::data ICU::uc OpenSSL::SSL Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Network Qt5::Multimedia Qt5::SerialPort Qt5::PrintSupport Qt5::Xml Qt5::TextToSpeech Qt5::Test QXmpp::QXmpp Threads::Threads SingleApplication::SingleApplication sentry::sentry Opus::opus leveldb kissfft ${LIBS})

#
# Qt Translations
# https://gist.github.com/02JanDal/70a39f2cc3d2002b3588
#
SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)
set(TS_BASE_FILES lang/translations/smallworld_en_US.ts)
# qt5_create_translation(QM_FILES ${GALAXY_UI_CPP} ${UI_HEADERS} ${GALAXY_OBJ_HEADERS} ${TS_BASE_FILES} OPTIONS -source-language en_US -no-obsolete)
# qt5_add_translation(QM_FILES ${TS_BASE_FILES})

#
# Create custom target for the generation and updating of translation files, as this tends to be a slow process otherwise
# that might tie up an otherwise normal compilation...
#
# add_custom_target(translations_update DEPENDS ${QM_FILES})

#
# The qm files are generated in the build tree, but the qrc file is inside the
# source directory and the path to resources are relative to the location of
# the qrc file itself. We use configure_file() to copy the qrc file in the build
# directory such that it can find the qm translations files. The qrc file is
# copied if it doesn't exist in the destination or if it is modified.
#
# configure_file(translations.qrc ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

qt5_add_binary_resources(resources src/sworld_res.qrc OPTIONS -no-compress)
add_dependencies(galaxy resources)

set(SWRLD_EXEC ${SWRLD_EXEC}
    src/main.cpp)

#
# Compile the primary executable for Small World Deluxe!
#
if(WIN32 OR MSYS OR MINGW)
    add_executable(smallworld WIN32 ${SWRLD_EXEC} ${QM_FILES})
elseif(LINUX)
    add_executable(smallworld ${SWRLD_EXEC} ${QM_FILES})
else()
    message(FATAL_ERROR "Unsupported operating system detected! Please check back at the official code repository for further updates. Thank you.")
endif()

target_compile_options(smallworld PRIVATE -fPIC) # Compile with position independent code!
target_compile_definitions(galaxy PRIVATE -DUSE_KISS_FFT)
target_link_libraries(smallworld PRIVATE galaxy) # Link with the 'Galaxy' set of libraries for good measure!

#
# Compile the automatic updater and checker for Small World Deluxe! It's a separate executable so that 
# end-user firewalls do not automatically block the primary for being network accessible.
#
add_executable(gk_update ${GK_AUTO_UPDATE_CPP} ${GK_AUTO_UPDATE_HEADERS} ${QM_FILES} ${GK_APP_VERS_HPP})
target_compile_options(gk_update PRIVATE -fPIC) # Compile with position independent code!
set_property(TARGET gk_update PROPERTY AUTOMOC ON)
target_link_libraries(gk_update PUBLIC ${ARIA2_LIBRARIES} ${LIBINTL_LIBRARIES} ${GETTEXT_LIBRARIES} ${ZLIB_LIBRARIES} ${LIBSSH2_LIBRARIES} ${ICU_LIBRARIES} ${Iconv_LIBRARIES} ${SQLite3_LIBRARIES} LibXml2::LibXml2 OpenSSL::SSL ${GK_EXTRA_LIBS} cares Qt5::Core Qt5::Network SingleApplication::SingleApplication)

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

    add_custom_command(TARGET smallworld
                    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_BINARY_DIR}/src/contrib/sentry-native/crashpad_build/handler/libcrashpad_handler_lib.a
                    ${CMAKE_CURRENT_BINARY_DIR}/libcrashpad_handler_lib.a)
endif()

if(WIN32 OR MSYS OR MINGW)
	if (EXISTS "$ENV{SystemDrive}/Program Files")
		INSTALL(TARGETS smallworld
				CONFIGURATIONS Debug
				RUNTIME DESTINATION Debug/bin
				LIBRARY DESTINATION Debug/lib)
		INSTALL(TARGETS smallworld
				CONFIGURATIONS Release
				RUNTIME DESTINATION bin
				LIBRARY DESTINATION lib)
	endif()
elseif (LINUX)
		INSTALL(TARGETS smallworld
				CONFIGURATIONS Debug
				RUNTIME DESTINATION Debug/bin
				LIBRARY DESTINATION Debug/lib)
		INSTALL(TARGETS smallworld
				CONFIGURATIONS Release
				RUNTIME DESTINATION bin
				LIBRARY DESTINATION lib)
else()
	message(FATAL_ERROR "Unsupported operating system detected! Please check back at the official code repository for further updates. Thank you.")
endif()

SET(CMAKE_C_FLAGS_DEBUG_INIT            "-g")
SET(CMAKE_C_FLAGS_MINSIZEREL_INIT       "-Os -DNDEBUG")
SET(CMAKE_C_FLAGS_RELEASE_INIT          "-O3 -DNDEBUG")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT   "-O2 -g")

SET(CMAKE_CXX_FLAGS_INIT                "-Wall")
SET(CMAKE_CXX_FLAGS_DEBUG_INIT          "-g")
SET(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "-Os -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE_INIT        "-O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")

if(MSYS OR MINGW OR LINUX)
    #
    # https://clang.llvm.org/docs/AddressSanitizer.html
    #
    if(GFYRE_ENBL_CLANG_ADDRESS_SANITIZER)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
        set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -lasan -static-libasan")
    endif()
    if (CMAKE_CXX_COMPILER_ID MATCHES "clang")
        #
        # Using regular Clang or AppleClang
        if(MSYS OR MINGW)
            set(CMAKE_C_COMPILER "/mingw64/bin/clang")
            set(CMAKE_CXX_COMPILER "/mingw64/bin/clang++")
        else()
            set(CMAKE_C_COMPILER "/usr/bin/clang")
            set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
        endif()
        add_compile_options(-Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context -Wno-missing-field-initializers)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        #
        # Using GCC
        if(MSYS OR MINGW)
            #
            # Nothing!
        else()
            set(CMAKE_C_COMPILER "/usr/bin/gcc")
            set(CMAKE_CXX_COMPILER "/usr/bin/g++")
        endif()
        add_compile_options(-Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-int-in-bool-context -Wno-missing-field-initializers)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        # Using Intel C++
    endif()
elseif(WIN32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # Using Microsoft Visual Studio C++
        if(CMAKE_BUILD_TYPE MATCHES "Debug")
            add_compile_options(\$<\$<CXX_COMPILER_ID:MSVC>:/W3 /WX)
        elseif(CMAKE_BUILD_TYPE MATCHES "Release")
            add_compile_options(\$<\$<CXX_COMPILER_ID:MSVC>:/W3 /WX /GL)
        endif()
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
