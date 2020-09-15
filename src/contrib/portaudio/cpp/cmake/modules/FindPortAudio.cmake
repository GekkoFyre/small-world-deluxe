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
#   [ 1 ] - https://code.gekkofyre.io/amateur-radio/small-world-deluxe
#

find_package(PkgConfig)
pkg_check_modules(PC_PortAudio QUIET portaudio)
set(PortAudio_DEFINITIONS ${PC_PortAudio_CFLAGS_OTHER})

find_path(PortAudio_INCLUDE_DIR NAMES "portaudio.h"
            HINTS ${PC_PortAudio_INCLUDE_DIR} ${PC_PortAudio_INCLUDE_DIRS}
            PATHS "/usr/local/include" "/usr/include")

find_library(PortAudio_LIBRARY NAMES "portaudio" "libportaudio" "portaudio_static_x64" "portaudio_x64" "portaudio_x86" "portaudio_static_x86"
            HINTS ${PC_PortAudio_LIBDIR} ${PC_PortAudio_LIBRARY_DIRS}
            PATHS "/usr/local/lib" "/usr/local/lib64" "/usr/lib" "/usr/lib64" "/mingw64/lib" "/mingw32/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortAudio DEFAULT_MSG PortAudio_LIBRARY PortAudio_INCLUDE_DIR)

mark_as_advanced(PortAudio_INCLUDE_DIR PortAudio_LIBRARY)

set(PortAudio_LIBRARIES ${PortAudio_LIBRARY})
set(PortAudio_INCLUDE_DIRS ${PortAudio_INCLUDE_DIR})
