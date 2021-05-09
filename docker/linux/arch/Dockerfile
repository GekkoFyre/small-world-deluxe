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
#   Copyright (C) 2020 - 2021. GekkoFyre.
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

#
# http://tutorials.jenkov.com/docker/dockerfile.html
#

#
# Some functionality was borrowed from the Dockerfile linked just below!
# Many thanks to Mykola Dimura <mykola.dimura@gmail.com> :)
# 
# https://hub.docker.com/r/burningdaylight/docker-mingw-qt5/dockerfile
#

ARG DOCKER_TAG=qt

# Use the official Arch Linux image as a parent image...
FROM archlinux:latest
MAINTAINER "Phobos A. D'thorga <phobos.gekko@gekkofyre.io>"

ENV EDITOR=nano

# Set the working directory...
WORKDIR /opt/gekkofyre/app

#
# Install the following packages and update the system!
#
RUN pacman-db-upgrade
RUN pacman -Sy --noconfirm
RUN pacman -Syyu --noconfirm
RUN pacman -S git --noconfirm
RUN pacman -S sudo --noconfirm
RUN pacman -S fakeroot --noconfirm
RUN pacman -S binutils --noconfirm
RUN pacman -S base-devel --noconfirm
RUN pacman -S nasm --noconfirm
RUN pacman -S nano --noconfirm
RUN pacman -S mlocate --noconfirm

# Create devel user...
RUN useradd -m -d /home/devel -u 1000 -U -G users,tty -s /bin/bash devel
RUN echo 'devel ALL=(ALL) NOPASSWD: /usr/sbin/pacman, /usr/sbin/makepkg' >> /etc/sudoers;

# Workaround for the "setrlimit(RLIMIT_CORE): Operation not permitted" error
RUN echo "Set disable_coredump false" >> /etc/sudo.conf

# Install yay
USER devel
ARG BUILDDIR=/tmp/tmp-build
RUN  mkdir "${BUILDDIR}" && cd "${BUILDDIR}" && \
     git clone https://aur.archlinux.org/yay.git && \
     cd yay && makepkg -si --noconfirm --rmdeps && \
     rm -rvf "${BUILDDIR}"

USER root

# Add mingw-repo
RUN    echo "[ownstuff]" >> /etc/pacman.conf \
    && echo "SigLevel = Optional TrustAll" >> /etc/pacman.conf \
    && echo "Server = https://martchus.no-ip.biz/repo/arch/ownstuff/os/\$arch" >> /etc/pacman.conf \
    && pacman -Sy

# Install essential MinGW packages (from the repository immediately above)
RUN pacman -S --noconfirm --noprogressbar \
        mingw-w64-binutils \
        mingw-w64-crt \
        mingw-w64-gcc \
        mingw-w64-headers \
        mingw-w64-winpthreads \
        mingw-w64-cmake \
        mingw-w64-tools \
        mingw-w64-zlib

# Cleanup
USER root
RUN pacman -Scc --noconfirm
RUN paccache -r -k0; \
    rm -rf /usr/share/man/*; \
    rm -rf /tmp/*; \
    rm -rf /var/tmp/*;
USER devel
RUN yay -Scc

ENV HOME=/home/devel

WORKDIR /opt/gekkofyre/app
ONBUILD USER root
ONBUILD WORKDIR /

USER root

# Install Qt5 and some other MinGW-related packages (from the aforementioned repository)
RUN pacman -S --noconfirm --noprogressbar \        
        mingw-w64-bzip2 \
        mingw-w64-expat \
        mingw-w64-fontconfig \
        mingw-w64-freeglut \
        mingw-w64-freetype2 \
        mingw-w64-gettext \
        mingw-w64-libdbus \
        mingw-w64-libiconv \
        mingw-w64-libjpeg-turbo \
        mingw-w64-libpng \
        mingw-w64-libtiff \
        mingw-w64-libxml2 \
        mingw-w64-mariadb-connector-c \
        mingw-w64-openssl \
        mingw-w64-openjpeg \
        mingw-w64-openjpeg2 \
        mingw-w64-pcre \
        mingw-w64-pdcurses \
        mingw-w64-pkg-config \
        mingw-w64-readline \
        mingw-w64-sdl2 \
        mingw-w64-sqlite \
        mingw-w64-termcap \
        mingw-w64-qt5-base \
        mingw-w64-qt5-declarative \
        mingw-w64-qt5-graphicaleffects \
        mingw-w64-qt5-imageformats \
        mingw-w64-qt5-location \
        mingw-w64-qt5-multimedia \
        mingw-w64-qt5-quickcontrols \
        mingw-w64-qt5-script \
        mingw-w64-qt5-sensors \
        mingw-w64-qt5-svg \
        mingw-w64-qt5-tools \
        mingw-w64-qt5-translations \
        mingw-w64-qt5-websockets \
        mingw-w64-qt5-winextras

# Install AUR packages
USER devel
RUN yay -S --noconfirm --noprogressbar --needed \
        mingw-w64-boost \
        mingw-w64-eigen \
        mingw-w64-qt5-quickcontrols2 \
        mingw-w64-qt5-serialport \
        mingw-w64-qt5-speech \
        mingw-w64-configure \
        mingw-w64-python-bin

# Cleanup
USER root
RUN pacman -Scc --noconfirm
RUN paccache -r -k0; \
    rm -rf /usr/share/man/*; \
    rm -rf /tmp/*; \
    rm -rf /var/tmp/*;
USER devel
RUN yay -Scc

WORKDIR /opt/gekkofyre/app
ONBUILD USER root
ONBUILD WORKDIR /

USER root
RUN pacman -Sy --noconfirm
RUN pacman -Syyu --noconfirm

RUN pacman -S gcc --noconfirm
RUN pacman -S gcc-fortran --noconfirm
RUN pacman -S octave --noconfirm
RUN pacman -S python --noconfirm
RUN pacman -S gdb --noconfirm
RUN pacman -S clang --noconfirm
RUN pacman -S llvm --noconfirm
RUN pacman -S llvm-libs --noconfirm
RUN pacman -S automake --noconfirm
RUN pacman -S autoconf --noconfirm
RUN pacman -S autoconf-archive --noconfirm
RUN pacman -S libtool --noconfirm
RUN pacman -S openssl --noconfirm
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
RUN pacman -S qt5-speech --noconfirm
RUN pacman -S zstd --noconfirm
RUN pacman -S libusb --noconfirm
RUN pacman -S hidapi --noconfirm
RUN pacman -S lz4 --noconfirm
RUN pacman -S zlib --noconfirm
RUN pacman -S leveldb --noconfirm
RUN pacman -S portaudio --noconfirm
RUN pacman -S libsndfile --noconfirm
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
RUN pacman -S cmake --noconfirm
RUN pacman -S wget --noconfirm
RUN pacman -S curl --noconfirm

USER devel
RUN yay -S conan --noconfirm

# Make sure to cleanup after all the updates and installations are completed!
USER root
RUN pacman -Scc --noconfirm

# Set the required environmental variables...
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

#
# Setup information for the Sentry server [ sentry.gekkofyre.io ]
#
CMD [ "curl", "-sL https://sentry.io/get-cli/ | bash" ]
CMD echo "[defaults]" >> ~/.sentryclirc
CMD echo "org=gekkofyre-networks" >> ~/.sentryclirc
CMD echo "project=small-world-deluxe" >> ~/.sentryclirc
CMD echo "url=https://sentry.gekkofyre.io/" >> ~/.sentryclirc

#
# Compile the following programs...
#
RUN [ "wget", "https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.16.tar.gz" ]
RUN tar -xvzf libiconv-1.16.tar.gz
CMD cd libiconv-1.16
CMD [ "./configure", "--enable-static --enable-shared" ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./..
CMD rm -rvf libiconv-1.16
CMD [ "git", "clone https://github.com/drowe67/codec2.git" ]
CMD cd codec2
CMD mkdir build
CMD cd build
CMD [ "cmake", "-G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=NO .." ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./../..
CMD rm -rvf codec2
RUN [ "wget", "https://github.com/hunspell/hunspell/files/2573619/hunspell-1.7.0.tar.gz" ]
RUN tar -xvzf hunspell-1.7.0.tar.gz
CMD cd hunspell-1.7.0
CMD autoreconf -vfi
CMD [ "./configure", "--enable-static --enable-shared" ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./..
CMD rm -rvf hunspell-1.7.0
CMD [ "git", "clone https://gitlab.xiph.org/xiph/libopusenc.git" ]
CMD cd libopusenc
CMD [ "git", "submodule update --init --recursive" ]
CMD sh autogen.sh
CMD [ "./configure", "--enable-static" ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./..
CMD rm -rvf libopusenc
CMD [ "git", "clone https://github.com/qxmpp-project/qxmpp.git" ]
CMD cd qxmpp
CMD [ "git", "submodule update --init --recursive" ]
CMD mkdir build
CMD cd build
CMD [ "cmake", "-G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=NO .." ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./../..
CMD rm -rvf qxmpp
CMD [ "git", "clone https://github.com/google/snappy.git" ]
CMD cd snappy
CMD [ "git", "submodule update --init --recursive" ]
CMD mkdir build
CMD cd build
CMD [ "cmake", "-G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DSNAPPY_BUILD_TESTS=OFF -DSNAPPY_BUILD_BENCHMARKS=OFF .." ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./../..
CMD rm -rvf snappy

#
# We had to login to the Docker image itself and compile this by hand manually!
#
RUN [ "wget", "https://github.com/Hamlib/Hamlib/releases/download/4.1/hamlib-4.1.tar.gz" ]
RUN tar -xvzf hamlib-4.1.tar.gz
CMD cd hamlib-4.1
CMD [ "./configure", "--enable-static --enable-shared --with-xml-support --with-cxx-binding --with-libusb" ]
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./..
CMD rm -rvf hamlib-4.1

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
CMD [ "make", "-j$(nproc)" ]
CMD [ "make", "install" ]
CMD cd ./../../../..
