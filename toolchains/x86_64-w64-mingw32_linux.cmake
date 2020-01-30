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
#   Copyright (C) 2019. GekkoFyre.
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

INCLUDE(CMakeForceCompiler)
 
SET(CMAKE_SYSTEM_NAME Windows)
# SET(CMAKE_SYSTEM_VERSION 1)

#
# Specify the cross-compiler
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling
# https://os.mbed.com/cookbook/mbed-cmake#cmake-toolchain-file
#
SET(CMAKE_C_COMPILER "x86_64-w64-mingw32-gcc")
SET(CMAKE_CXX_COMPILER "x86_64-w64-mingw32-g++")
SET(CMAKE_RC_COMPILER "x86_64-w64-mingw32-windres")

macro(use_cxx17)
    if(CMAKE_VERSION VERSION_LESS "3.1")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # SET(COMMON_FLAGS "-mcpu=cortex-m3 -mthumb -mthumb-interwork -msoft-float -ffunction-sections -fdata-sections -g -fno-common -fmessage-length=0")
            SET(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=gnu++17")
            SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu99")
            SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-gc-sections ")
        endif()
    else()
        set(CMAKE_CXX_STANDARD 17)
    endif()
endmacro(use_cxx17)

# Here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

#
# Adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
#
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)