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

# Download base image and add the proper labels
FROM burningdaylight/docker-mingw-qt5:latest
LABEL maintainer="Phobos D'thorga <phobos.gekko@gekkofyre.io>"
LABEL vendor="GekkoFyre Networks"
LABEL version="1.0"
LABEL description="See our GitLab repository for more details: <https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe>"

# Check for updates and if any, do an upgrade...
RUN pacman -Sy --noconfirm --needed
RUN pacman -Syu --noconfirm --needed

RUN sudo 'yay -S --noconfirm "mingw-w64-zstd"'
RUN sudo 'yay -S --noconfirm "mingw-w64-lz4"'
RUN sudo 'yay -S --noconfirm "mingw-w64-zlib"'
RUN sudo 'yay -S --noconfirm "mingw-w64-leveldb"'
RUN sudo 'yay -S --noconfirm "mingw-w64-portaudio"'
RUN sudo 'yay -S --noconfirm "mingw-w64-winpthreads"'
RUN sudo 'yay -S --noconfirm "mingw-w64-libusb"'
RUN sudo 'yay -S --noconfirm "mingw-w64-gflags"'
RUN sudo 'yay -S --noconfirm "mingw-w64-fftw"'
RUN sudo 'yay -S --noconfirm "mingw-w64-qwt"'
RUN sudo 'yay -S --noconfirm "mingw-w64-libogg"'
RUN sudo 'yay -S --noconfirm "mingw-w64-libvorbis"'

# Cleanup any unneeded temporary files on the system
RUN sudo 'yay -Sc --noconfirm'
RUN pacman -Sc --noconfirm

# Make sure to generate development package database used for devel update
RUN sudo 'yay -Y --noconfirm --gendb'

# Print system statistics before finally signing-off!
RUN yay -Ps --noconfirm