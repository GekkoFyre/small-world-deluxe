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
#   Copyright (C) 2020 - 2022. GekkoFyre.
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

find_package(PkgConfig)
pkg_check_modules(PC_ARIA2 QUIET "aria2")
set(ARIA2_DEFINITIONS ${PC_ARIA2_CFLAGS_OTHER})

find_path(ARIA2_INCLUDE_DIR
    NAMES "aria2/aria2.h"
    HINTS ${PC_ARIA2_INCLUDE_DIR} ${PC_ARIA2_INCLUDE_DIRS}
    PATHS "/usr/local/include" "/usr/include" "/opt/local/include" "/mingw64/include")

find_library(ARIA2_LIBRARY
    NAMES "libaria2" "aria2" "libaria2-0"
    HINTS ${PC_ARIA2_LIBDIR} ${PC_ARIA2_LIBRARY_DIRS}
    PATHS "/usr/local/lib" "/usr/local/lib64" "/usr/lib" "/usr/lib64" "/mingw64/bin" "/mingw64/lib" "/usr/lib/x86_64-linux-gnu" "/sw/lib" "/opt/local/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Aria2 DEFAULT_MSG ARIA2_LIBRARY ARIA2_INCLUDE_DIR)

mark_as_advanced(ARIA2_INCLUDE_DIR ARIA2_LIBRARY)

set(ARIA2_LIBRARIES ${ARIA2_LIBRARY})
set(ARIA2_INCLUDE_DIRS ${ARIA2_INCLUDE_DIR})
