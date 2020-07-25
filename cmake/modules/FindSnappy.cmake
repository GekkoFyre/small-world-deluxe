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

find_package(PkgConfig)
pkg_check_modules(PC_SNAPPY QUIET "snappy")
set(SNAPPY_DEFINITIONS ${PC_SNAPPY_CFLAGS_OTHER})

find_path(SNAPPY_INCLUDE_DIR
    NAMES "snappy.h"
    HINTS ${PC_SNAPPY_INCLUDE_DIR} ${PC_SNAPPY_INCLUDE_DIRS}
    PATHS "/usr/local/include" "/usr/include")

find_library(SNAPPY_LIBRARY
    NAMES  "snappy" "libsnappy"
    HINTS ${PC_SNAPPY_LIBDIR} ${PC_SNAPPY_LIBRARY_DIRS}
    PATHS "/usr/local/lib" "/usr/local/lib64" "/usr/lib" "/usr/lib64")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Snappy DEFAULT_MSG SNAPPY_LIBRARY SNAPPY_INCLUDE_DIR)

mark_as_advanced(SNAPPY_INCLUDE_DIR SNAPPY_LIBRARY)

set(SNAPPY_LIBRARIES ${SNAPPY_LIBRARY})
set(SNAPPY_INCLUDE_DIRS ${SNAPPY_INCLUDE_DIR})
