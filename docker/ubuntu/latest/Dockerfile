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

# https://hub.docker.com/_/ubuntu
# docker build -t gekkofyre/ubuntu:latest .
# docker push gekkofyre/ubuntu:latest
# docker login --username=yourhubusername --password=yourpassword
# latest: digest: sha256:c899b6f13c580ffd9d115fdcb08999c509f5b8235604104d08c7fc73f4e04425 size: 2833

# Download base image and add the proper labels
FROM ubuntu:disco
LABEL maintainer="Phobos D'thorga <phobos.gekko@gekkofyre.io>"
LABEL vendor="GekkoFyre Networks"
LABEL version="1.0"
LABEL description="See our GitLab repository for more details: <https://git.gekkofyre.io/amateur-radio/small-world-deluxe/tree/develop/docker>"

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get clean all -y && apt-get update -y
RUN apt-get install -y "apt-utils" "software-properties-common"
RUN apt-get clean all -y && apt-get update -y
RUN apt-get upgrade -y
RUN apt-get install -y "ca-certificates"
RUN apt-get install -y "build-essential"
RUN apt-get install -y "libtool" "libtool-bin" "libltdl-dev" "libc6" "libltdl7" "sysconftool" "automake" "autoconf" "autotools-dev" "cmake" "cmake-data" "cmake-extras" "libboost-all-dev" "libzstd-dev" "liblz4-dev" "libsnappy-dev" "zlibc" "zlib1g-dev" "libicu-dev" "icu-devtools" "libcurlpp-dev" "libcurl4-openssl-dev" "libcurl4" "curl" "wget" "wget2-dev" "libgflags-dev" "qt5-qmake" "libqt5charts5-dev" "libqt5waylandcompositor5-dev" "libqt5waylandcompositor5" "libqt5waylandclient5-dev" "libqt5waylandclient5" "qt5-default" "git-all" "libleveldb-dev" "libleveldb1d" "libqwt-qt5-dev" "libqwt-headers" "libqt5widgets5" "libqt5multimediawidgets5" "libqt5core5a" "libqt53dcore5" "libqt5test5" "libqt5gui5" "libgwengui-qt5-dev" "qtwayland5" "qtwayland5-dev-tools" "texinfo" "libusb-1.0-0" "libstdc++6" "libgcc1" "libportaudio2" "libportaudiocpp0" "portaudio19-dev" "libjack0" "libjack-dev" "libasound2-dev" "jackd1" "perl"
RUN apt-get clean all -y