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

find_package(PkgConfig)
pkg_check_modules(PC_LIBLZ4 QUIET liblz4)
set(LIBLZ4_DEFINITIONS ${PC_LIBLZ4_CFLAGS_OTHER})

find_path(LIBLZ4_INCLUDE_DIR NAMES "lz4.h" "lz4hc.h"
            HINTS ${PC_LIBLZ4_INCLUDEDIR} ${PC_LIBLZ4_INCLUDE_DIRS}
            PATH_SUFFIXES liblz4)

find_library(LIBLZ4_LIBRARY NAMES "lz4_static" "liblz4_static" "lz4" "liblz4"
            HINTS ${PC_LIBLZ4_LIBDIR} ${PC_LIBLZ4_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LZ4 DEFAULT_MSG LIBLZ4_LIBRARY LIBLZ4_INCLUDE_DIR)

mark_as_advanced(LIBLZ4_INCLUDE_DIR LIBLZ4_LIBRARY)

set(LIBLZ4_LIBRARIES ${LIBLZ4_LIBRARY})
set(LIBLZ4_INCLUDE_DIRS ${LIBLZ4_INCLUDE_DIR})