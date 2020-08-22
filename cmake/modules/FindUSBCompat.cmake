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
pkg_check_modules(PC_LIBUSB_COMPAT QUIET libusb_compat)
set(LIBUSB_COMPAT_DEFINITIONS ${PC_LIBUSB_COMPAT_CFLAGS_OTHER})

find_path(LIBUSB_COMPAT_INCLUDE_DIR
            NAMES "usb.h"
            HINTS ${PC_LIBUSB_COMPAT_INCLUDE_DIR} ${PC_LIBUSB_COMPAT_INCLUDE_DIRS}
            PATHS "/usr/local/include" "/usr/include")

find_library(LIBUSB_COMPAT_LIBRARY NAMES "usb" "libusb"
            HINTS ${PC_LIBUSB_COMPAT_LIBDIR} ${PC_LIBUSB_COMPAT_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(USB_COMPAT DEFAULT_MSG LIBUSB_COMPAT_LIBRARY LIBUSB_COMPAT_INCLUDE_DIR)

mark_as_advanced(LIBUSB_COMPAT_INCLUDE_DIR LIBUSB_COMPAT_LIBRARY)

set(LIBUSB_COMPAT_LIBRARIES ${LIBUSB_COMPAT_LIBRARY})
set(LIBUSB_COMPAT_INCLUDE_DIRS ${LIBUSB_COMPAT_INCLUDE_DIR})
