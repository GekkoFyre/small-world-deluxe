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

option(GFYRE_PACKAGE_MAINTAINER "Are you a package maintainer? If so, set this option to `ON`! It'll enable specific build settings to make your life easier." ON)

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

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

SET(PA_COMMON_INCLUDES
    ./../../src/common/pa_allocation.h
    ./../../src/common/pa_converters.h
    ./../../src/common/pa_cpuload.h
    ./../../src/common/pa_debugprint.h
    ./../../src/common/pa_dither.h
    ./../../src/common/pa_endianness.h
    ./../../src/common/pa_hostapi.h
    ./../../src/common/pa_memorybarrier.h
    ./../../src/common/pa_process.h
    ./../../src/common/pa_ringbuffer.h
    ./../../src/common/pa_stream.h
    ./../../src/common/pa_trace.h
    ./../../src/common/pa_types.h
    ./../../src/common/pa_util.h
)

SET(PA_COMMON_SOURCES
    ./../../src/common/pa_allocation.c
    ./../../src/common/pa_converters.c
    ./../../src/common/pa_cpuload.c
    ./../../src/common/pa_debugprint.c
    ./../../src/common/pa_dither.c
    ./../../src/common/pa_front.c
    ./../../src/common/pa_process.c
    ./../../src/common/pa_ringbuffer.c
    ./../../src/common/pa_stream.c
    ./../../src/common/pa_trace.c
)

SOURCE_GROUP("common" FILES ${PA_COMMON_INCLUDES} ${PA_COMMON_SOURCES})

SET(PA_PUBLIC_INCLUDES ./../../include/portaudio.h)

SET(PA_SKELETON_SOURCES ./../../src/hostapi/skeleton/pa_hostapi_skeleton.c)
SOURCE_GROUP("hostapi\\skeleton" ${PA_SKELETON_SOURCES})
SET(PA_SOURCES ${PA_COMMON_SOURCES} ${PA_SKELETON_SOURCES})
SET(PA_PRIVATE_INCLUDE_PATHS ./../../src/common ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} _CRT_SECURE_NO_WARNINGS)
    SET(PA_PLATFORM_SOURCES
        ./../../src/os/win/pa_win_hostapis.c
        ./../../src/os/win/pa_win_util.c
        ./../../src/os/win/pa_win_waveformat.c
        ./../../src/os/win/pa_win_wdmks_utils.c
        ./../../src/os/win/pa_win_coinitialize.c)
    SET(PA_PLATFORM_INCLUDES
        ./../../src/os/win/pa_win_coinitialize.h
        ./../../src/os/win/pa_win_wdmks_utils.h)

    IF(MSVC)
        SET(PA_PLATFORM_SOURCES ${PA_PLATFORM_SOURCES} ./../../src/os/win/pa_x86_plain_converters.c)
        SET(PA_PLATFORM_INCLUDES ${PA_PLATFORM_INCLUDES} ./../../src/os/win/pa_x86_plain_converters.h)
    ELSE()
        SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} _WIN32_WINNT=0x0501 WINVER=0x0501)
        SET(DEF_EXCLUDE_X86_PLAIN_CONVERTERS ";")
    ENDIF()

    SOURCE_GROUP("os\\win" FILES ${PA_PLATFORM_SOURCES} ${PA_PLATFORM_INCLUDES})
    SET(PA_SOURCES ${PA_SOURCES} ${PA_PLATFORM_SOURCES})
    SET(PA_PRIVATE_INCLUDES ${PA_PRIVATE_INCLUDES} ${PA_PLATFORM_INCLUDES})
    SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} ./../../src/os/win)

    SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} winmm)
    
    # Try to find ASIO SDK (assumes that portaudio and asiosdk folders are side-by-side, see
    # http://www.portaudio.com/trac/wiki/TutorialDir/Compile/WindowsASIOMSVC)
    FIND_PACKAGE(ASIOSDK)
    IF(ASIOSDK_FOUND)
        OPTION(PA_USE_ASIO "Enable support for ASIO" ON)
    ELSE()
        OPTION(PA_USE_ASIO "Enable support for ASIO" OFF)
    ENDIF()
    IF(PA_USE_ASIO)
        SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} ${ASIOSDK_ROOT_DIR}/common)
        SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} ${ASIOSDK_ROOT_DIR}/host)
        SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} ${ASIOSDK_ROOT_DIR}/host/pc)
        SET(PA_ASIO_SOURCES src/hostapi/asio/pa_asio.cpp src/hostapi/asio/iasiothiscallresolver.cpp)
        SET(PA_ASIOSDK_SOURCES ${ASIOSDK_ROOT_DIR}/common/asio.cpp ${ASIOSDK_ROOT_DIR}/host/pc/asiolist.cpp ${ASIOSDK_ROOT_DIR}/host/asiodrivers.cpp)
        SOURCE_GROUP("hostapi\\ASIO" FILES ${PA_ASIO_SOURCES})
        SOURCE_GROUP("hostapi\\ASIO\\ASIOSDK" FILES ${PA_ASIOSDK_SOURCES})
        SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_asio.h)
        SET(PA_SOURCES ${PA_SOURCES} ${PA_ASIO_SOURCES})
        SET(PA_NON_UNICODE_SOURCES ${PA_NON_UNICODE_SOURCES} ${PA_ASIOSDK_SOURCES})
        SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} ole32 uuid)
    ELSE()
        # Set variables for DEF file expansion
        SET(DEF_EXCLUDE_ASIO_SYMBOLS ";")
    ENDIF()

    OPTION(PA_USE_DS "Enable support for DirectSound" ON)
    IF(PA_USE_DS)
        IF(MINGW)
            MESSAGE(STATUS "DirectSound support will be built with DSound provided by MinGW.")
            OPTION(PA_USE_DIRECTSOUNDFULLDUPLEXCREATE "Use DirectSound full duplex create" OFF)
        ELSE(MINGW)
            OPTION(PA_USE_DIRECTSOUNDFULLDUPLEXCREATE "Use DirectSound full duplex create" ON)
        ENDIF(MINGW)
        MARK_AS_ADVANCED(PA_USE_DIRECTSOUNDFULLDUPLEXCREATE)
        IF(PA_USE_DIRECTSOUNDFULLDUPLEXCREATE)
            SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PAWIN_USE_DIRECTSOUNDFULLDUPLEXCREATE)
        ENDIF()
        SET(PA_DS_INCLUDES src/hostapi/dsound/pa_win_ds_dynlink.h)
        SET(PA_DS_SOURCES src/hostapi/dsound/pa_win_ds.c src/hostapi/dsound/pa_win_ds_dynlink.c)
        SOURCE_GROUP("hostapi\\dsound" FILES ${PA_DS_INCLUDES} ${PA_DS_SOURCES})
        SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_win_ds.h ./../../include/pa_win_waveformat.h)
        SET(PA_PRIVATE_INCLUDES ${PA_PRIVATE_INCLUDES} ${PA_DS_INCLUDES})
        SET(PA_SOURCES ${PA_SOURCES} ${PA_DS_SOURCES})
        SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} dsound)
    ENDIF(PA_USE_DS)

    OPTION(PA_USE_WMME "Enable support for MME" ON)
    IF(PA_USE_WMME)
        SET(PA_WMME_SOURCES src/hostapi/wmme/pa_win_wmme.c)
        SOURCE_GROUP("hostapi\\wmme" FILES ${PA_WMME_SOURCES})
        SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_win_wmme.h ./../../include/pa_win_waveformat.h)
        SET(PA_SOURCES ${PA_SOURCES} ${PA_WMME_SOURCES})
        SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} ole32 uuid)
    ENDIF()

    # MinGW versions below 4.93, especially non MinGW-w64 distributions may
    # break in the wasapi build. If an older MinGW version is required, WASAPI-
    # support needs to be disabled.
    OPTION(PA_USE_WASAPI "Enable support for WASAPI" ON)
    IF(PA_USE_WASAPI)
        SET(PA_WASAPI_SOURCES src/hostapi/wasapi/pa_win_wasapi.c)
        SOURCE_GROUP("hostapi\\wasapi" FILES ${PA_WASAPI_SOURCES})
        SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_win_wasapi.h ./../../include/pa_win_waveformat.h)
        SET(PA_SOURCES ${PA_SOURCES} ${PA_WASAPI_SOURCES})
        SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} ole32 uuid)
    ELSE()
        SET(DEF_EXCLUDE_WASAPI_SYMBOLS ";")
    ENDIF()

    OPTION(PA_USE_WDMKS "Enable support for WDMKS" ON)
    IF(PA_USE_WDMKS)
        SET(PA_WDMKS_SOURCES src/hostapi/wdmks/pa_win_wdmks.c)
        SOURCE_GROUP("hostapi\\wdmks" FILES ${PA_WDMKS_SOURCES})
        SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_win_wdmks.h)
        SET(PA_SOURCES ${PA_SOURCES} ${PA_WDMKS_SOURCES})
        SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} setupapi ole32 uuid)
    ENDIF()

    OPTION(PA_USE_WDMKS_DEVICE_INFO "Use WDM/KS API for device info" ON)
    MARK_AS_ADVANCED(PA_USE_WDMKS_DEVICE_INFO)
    IF(PA_USE_WDMKS_DEVICE_INFO)
        SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PAWIN_USE_WDMKS_DEVICE_INFO)
    ENDIF()

    SET(GENERATED_MESSAGE "CMake generated file, do NOT edit! Use CMake-GUI to change configuration instead.")
    CONFIGURE_FILE(cmake_support/template_portaudio.def ${CMAKE_CURRENT_BINARY_DIR}/portaudio_cmake.def @ONLY)
    CONFIGURE_FILE(cmake_support/options_cmake.h.in ${CMAKE_CURRENT_BINARY_DIR}/options_cmake.h @ONLY)
    SET(PA_PRIVATE_INCLUDES ${PA_PRIVATE_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR}/options_cmake.h)
    SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PORTAUDIO_CMAKE_GENERATED)
    SOURCE_GROUP("cmake_generated" FILES ${CMAKE_CURRENT_BINARY_DIR}/portaudio_cmake.def ${CMAKE_CURRENT_BINARY_DIR}/options_cmake.h)

    SET(PA_EXTRA_SHARED_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/portaudio_cmake.def)

ELSE()

    SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} src/os/unix)
    SET(PA_PLATFORM_SOURCES ./../../src/os/unix/pa_unix_hostapis.c ./../../src/os/unix/pa_unix_util.c)
    SOURCE_GROUP("os\\unix" FILES ${PA_PLATFORM_SOURCES})
    SET(PA_SOURCES ${PA_SOURCES} ${PA_PLATFORM_SOURCES})

    IF(APPLE)
        message(SEND_ERROR "We do not currently support compilation for Apple Mac OS/X systems at this stage. Sorry!")
    ELSEIF(UNIX)

        FIND_PACKAGE(Jack)
        IF(JACK_FOUND)
            OPTION(PA_USE_JACK "Enable support for Jack" ON)
        ELSE()
            OPTION(PA_USE_JACK "Enable support for Jack" OFF)
        ENDIF()
        IF(PA_USE_JACK)
            SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} ${JACK_INCLUDE_DIRS})
            SET(PA_JACK_SOURCES ./../../src/hostapi/jack/pa_jack.c)
            SOURCE_GROUP("hostapi\\JACK" FILES ${PA_JACK_SOURCES})
            SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_jack.h)
            SET(PA_SOURCES ${PA_SOURCES} ${PA_JACK_SOURCES})
            SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PA_USE_JACK)
            SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} ${JACK_LIBRARIES})
            SET(PA_PKGCONFIG_LDFLAGS "${PA_PKGCONFIG_LDFLAGS} -ljack")
        ENDIF()

        FIND_PACKAGE(ALSA)
        IF(ALSA_FOUND)
            OPTION(PA_USE_ALSA "Enable support for ALSA" ON)
        ELSE()
            OPTION(PA_USE_ALSA "Enable support for ALSA" OFF)
        ENDIF()
        IF(PA_USE_ALSA)
            SET(PA_PRIVATE_INCLUDE_PATHS ${PA_PRIVATE_INCLUDE_PATHS} ${ALSA_INCLUDE_DIRS})
            SET(PA_ALSA_SOURCES ./../../src/hostapi/alsa/pa_linux_alsa.c)
            SOURCE_GROUP("hostapi\\ALSA" FILES ${PA_ALSA_SOURCES})
            SET(PA_PUBLIC_INCLUDES ${PA_PUBLIC_INCLUDES} ./../../include/pa_linux_alsa.h)
            SET(PA_SOURCES ${PA_SOURCES} ${PA_ALSA_SOURCES})
            SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PA_USE_ALSA)
            SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} ${ALSA_LIBRARIES})
            SET(PA_PKGCONFIG_LDFLAGS "${PA_PKGCONFIG_LDFLAGS} -lasound")
        ENDIF()

    ENDIF()
    SET(PA_PKGCONFIG_LDFLAGS "${PA_PKGCONFIG_LDFLAGS} -lm -lpthread")
    SET(PA_LIBRARY_DEPENDENCIES ${PA_LIBRARY_DEPENDENCIES} m pthread)

ENDIF()

SOURCE_GROUP("./../../include" FILES ${PA_PUBLIC_INCLUDES})
SET(PA_INCLUDES ${PA_PRIVATE_INCLUDES} ${PA_PUBLIC_INCLUDES})

OPTION(PA_ENABLE_DEBUG_OUTPUT "Enable debug output for Portaudio" OFF)
IF(PA_ENABLE_DEBUG_OUTPUT)
    SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PA_ENABLE_DEBUG_OUTPUT)
ENDIF()

INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
IF(IS_BIG_ENDIAN)
    SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PA_BIG_ENDIAN)
ELSE()
    SET(PA_PRIVATE_COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS} PA_LITTLE_ENDIAN)
ENDIF()

OPTION(PA_BUILD_STATIC "Build static library" ON)
OPTION(PA_BUILD_SHARED "Build shared/dynamic library" ON)

IF(MSVC)
    OPTION(PA_LIBNAME_ADD_SUFFIX "Add suffix _static to static library name" ON)
ELSE()
    OPTION(PA_LIBNAME_ADD_SUFFIX "Add suffix _static to static library name" OFF)
ENDIF()

# MSVC: if PA_LIBNAME_ADD_SUFFIX is not used, and both static and shared libraries are
# built, one, of import- and static libraries, will overwrite the other. In
# embedded builds this is not an issue as they will only build the configuration
# used in the host application.
MARK_AS_ADVANCED(PA_LIBNAME_ADD_SUFFIX)
IF(MSVC AND PA_BUILD_STATIC AND PA_BUILD_SHARED AND NOT PA_LIBNAME_ADD_SUFFIX)
    MESSAGE(WARNING "Building both shared and static libraries, and avoiding the suffix _static will lead to a name conflict")
    SET(PA_LIBNAME_ADD_SUFFIX ON CACHE BOOL "Forcing use of suffix _static to avoid name conflict between static and import library" FORCE)
    MESSAGE(WARNING "PA_LIBNAME_ADD_SUFFIX was set to ON")
ENDIF()

SET(PA_TARGETS "")

IF(WIN32 AND MSVC)
    OPTION(PA_CONFIG_LIB_OUTPUT_PATH "Make sure that output paths are kept neat" OFF)
    IF(CMAKE_CL_64)
        SET(TARGET_POSTFIX x64)
        IF(PA_CONFIG_LIB_OUTPUT_PATH)
            SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/x64)
        ENDIF()
    ELSE()
        SET(TARGET_POSTFIX x86)
        IF(PA_CONFIG_LIB_OUTPUT_PATH)
            SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/Win32)
        ENDIF()
    ENDIF()
    IF(PA_BUILD_SHARED)
        IF(PA_LIBNAME_ADD_SUFFIX)
            SET_TARGET_PROPERTIES(portaudio PROPERTIES OUTPUT_NAME portaudio_${TARGET_POSTFIX})
        ELSE()
            SET_TARGET_PROPERTIES(portaudio PROPERTIES OUTPUT_NAME portaudio)
        ENDIF()
    ENDIF()
    IF(PA_BUILD_STATIC)
        IF(PA_LIBNAME_ADD_SUFFIX)
            SET_TARGET_PROPERTIES(portaudio_static PROPERTIES OUTPUT_NAME portaudio_static_${TARGET_POSTFIX})
        ELSE()
            SET_TARGET_PROPERTIES(portaudio_static PROPERTIES OUTPUT_NAME portaudio)
        ENDIF()
    ENDIF()
ELSE()
    IF(APPLE AND CMAKE_VERSION VERSION_GREATER 3.4.2)
        OPTION(PA_OUTPUT_OSX_FRAMEWORK "Generate an OS X framework instead of the simple library" OFF)
        IF(PA_OUTPUT_OSX_FRAMEWORK)
            SET_TARGET_PROPERTIES(portaudio PROPERTIES
            FRAMEWORK TRUE
            MACOSX_FRAMEWORK_IDENTIFIER com.portaudio
            FRAMEWORK_VERSION A
            PUBLIC_HEADER "${PA_PUBLIC_INCLUDES}"
            VERSION ${PA_SOVERSION}
            SOVERSION ${PA_SOVERSION})
        ENDIF()
    ENDIF()
ENDIF()

# Prepared for inclusion of test files
OPTION(PA_BUILD_TESTS "Include test projects" OFF)
IF(PA_BUILD_TESTS)
    SUBDIRS(test)
ENDIF()

# Prepared for inclusion of test files
OPTION(PA_BUILD_EXAMPLES "Include example projects" OFF)
IF(PA_BUILD_EXAMPLES)
    SUBDIRS(examples)
ENDIF()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSYS" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MINGW" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(GALAXY_SRC_CPP
        src/AsioDeviceAdapter.cxx
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

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSYS" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MINGW")
    set(GALAXY_SRC_HEADERS
        include/portaudiocpp/AsioDeviceAdapter.hxx
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

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

IF(PA_BUILD_STATIC)
    LIST(APPEND PA_TARGETS portaudio_static)
    add_library(portaudio_static STATIC ${PA_INCLUDES} ${PA_COMMON_INCLUDES} ${PA_SOURCES} ${PA_NON_UNICODE_SOURCES})
    set_property(TARGET portaudio_static APPEND_STRING PROPERTY COMPILE_DEFINITIONS ${PA_PRIVATE_COMPILE_DEFINITIONS})
    target_include_directories(portaudio_static PRIVATE ${PA_PRIVATE_INCLUDE_PATHS})
    target_include_directories(portaudio_static PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>")
    TARGET_LINK_LIBRARIES(portaudio_static ${PA_LIBRARY_DEPENDENCIES})
    IF(NOT PA_LIBNAME_ADD_SUFFIX)
        SET_PROPERTY(TARGET portaudio_static PROPERTY OUTPUT_NAME portaudio)
    ENDIF()
ENDIF()

add_library(portaudiocpp_static STATIC ${GALAXY_SRC_CPP} ${GALAXY_SRC_HEADERS})
target_include_directories(portaudiocpp_static PUBLIC include)
target_link_libraries(portaudiocpp_static PUBLIC portaudio_static)
set_target_properties(portaudiocpp_static PROPERTIES OUTPUT_NAME portaudiocpp)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Using regular Clang or AppleClang
    target_compile_options(portaudio_static PUBLIC -Wno-unused-variable)
    target_compile_options(portaudiocpp_static PUBLIC -Wno-unused-variable)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Using GCC
    target_compile_options(portaudio_static PUBLIC -Wno-unused-variable)
    target_compile_options(portaudiocpp_static PUBLIC -Wno-unused-variable)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MINGW")
    # Using MinGW
    target_compile_options(portaudio_static PUBLIC -Wno-unused-variable)
    target_compile_options(portaudiocpp_static PUBLIC -Wno-unused-variable)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # Using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # Using Microsoft Visual Studio C++
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(portaudio_static PUBLIC "/W1")
    target_compile_options(portaudiocpp_static PUBLIC "/W1")
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(portaudio_static PUBLIC "/W1 /GL")
    target_compile_options(portaudiocpp_static PUBLIC "/W1 /GL")
    endif()
endif()
