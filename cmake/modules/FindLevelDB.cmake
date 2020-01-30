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

find_package(PkgConfig)
pkg_check_modules(PC_LEVELDB QUIET leveldb)
set(LEVELDB_DEFINITIONS ${PC_LEVELDB_CFLAGS_OTHER})

find_path(LEVELDB_INCLUDE_DIR NAMES "leveldb/db.h"
            HINTS ${PC_LEVELDB_INCLUDEDIR} ${PC_LEVELDB_INCLUDE_DIRS}
            PATH_SUFFIXES leveldb)

find_library(LEVELDB_LIBRARY NAMES "leveldb" "libleveldb"
            HINTS ${PC_LEVELDB_LIBDIR} ${PC_LEVELDB_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LEVELDB DEFAULT_MSG LEVELDB_LIBRARY LEVELDB_INCLUDE_DIR)

mark_as_advanced(LEVELDB_INCLUDE_DIR LEVELDB_LIBRARY)

set(LEVELDB_LIBRARIES ${LEVELDB_LIBRARY})
set(LEVELDB_INCLUDE_DIRS ${LEVELDB_INCLUDE_DIR})