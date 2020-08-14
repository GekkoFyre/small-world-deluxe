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

#
# http://tutorials.jenkov.com/docker/dockerfile.html
#

# Use the official Arch Linux image as a parent image...
FROM archlinux:latest

# Set the working directory...
WORKDIR /opt/gekkofyre/app

#
# Install the following packages and update the system!
#
RUN pacman -Sy --noconfirm
RUN pacman -Syu --noconfirm
RUN pacman -S base-devel --noconfirm
RUN pacman -S gcc --noconfirm
RUN pacman -S gcc-fortran --noconfirm
RUN pacman -S octave --noconfirm
RUN pacman -S python --noconfirm
RUN pacman -S gdb --noconfirm
RUN pacman -S clang --noconfirm
RUN pacman -S llvm --noconfirm
RUN pacman -S llvm-libs --noconfirm
RUN pacman -S binutils --noconfirm
RUN pacman -S automake --noconfirm
RUN pacman -S autoconf --noconfirm
RUN pacman -S autoconf-archive --noconfirm
RUN pacman -S boost --noconfirm
RUN pacman -S qt5-base --noconfirm
RUN pacman -S qt5-tools --noconfirm
RUN pacman -S qt5-serialport --noconfirm
RUN pacman -S qt5-sensors --noconfirm
RUN pacman -S qt5-imageformats --noconfirm
RUN pacman -S qt5-translations --noconfirm
RUN pacman -S qt5-svg --noconfirm
RUN pacman -S qt5-multimedia --noconfirm
RUN pacman -S qt5-xmlpatterns --noconfirm
RUN pacman -S qt5-charts --noconfirm
RUN pacman -S qt5-3d --noconfirm
RUN pacman -S qt5-connectivity --noconfirm
RUN pacman -S zstd --noconfirm
RUN pacman -S libusb --noconfirm
RUN pacman -S hidapi --noconfirm
RUN pacman -S lz4 --noconfirm
RUN pacman -S zlib --noconfirm
RUN pacman -S leveldb --noconfirm
RUN pacman -S portaudio --noconfirm
RUN pacman -S libusb --noconfirm
RUN pacman -S fftw --noconfirm
RUN pacman -S snappy --noconfirm
RUN pacman -S qwt --noconfirm
RUN pacman -S libogg --noconfirm
RUN pacman -S libvorbis --noconfirm
RUN pacman -S opus --noconfirm
RUN pacman -S opusfile --noconfirm
RUN pacman -S texinfo --noconfirm
RUN pacman -S libusb-compat --noconfirm
RUN pacman -S hamlib --noconfirm
RUN pacman -S git --noconfirm
RUN pacman -S cmake --noconfirm
RUN pacman -S wget --noconfirm

# Make sure to cleanup after all the updates and installations are completed!
RUN pacman -Sc --noconfirm

# Set the required environmental variables...
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

#
# Compile the following programs...
#
RUN [ "wget", "https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.16.tar.gz" ]
RUN tar -xvzf libiconv-1.16.tar.gz
CMD cd libiconv-1.16
CMD [ "./configure", "--enable-static --enable-shared" ]
CMD [ "make", "-j 12" ]
CMD [ "make", "install" ]
CMD cd ./..
CMD rm -rvf libiconv-1.16
CMD [ "git", "clone https://github.com/drowe67/codec2.git" ]
CMD cd codec2
CMD mkdir build
CMD cd build
CMD [ "cmake", "-G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=NO .." ]
CMD [ "make", "-j 12" ]
CMD [ "make", "install" ]
CMD cd ./../..
CMD rm -rvf codec2

#
# Make it so that the Qt5 libraries (i.e. specifically `libQt5Core.so.5`) can run with Small World Deluxe, and thusly 
# this version of Arch Linux's package!
#
RUN strip --remove-section=.note.ABI-tag /usr/lib64/libQt5Core.so.5

#
# Compile the final library required!
#
CMD cd src/contrib/QtUsb
CMD mkdir build
CMD cd build
CMD [ "qmake", "-makefile .." ]
CMD [ "make", "-j 12" ]
CMD [ "make", "install" ]
CMD cd ./../../../..
