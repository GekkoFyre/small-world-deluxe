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

#
# https://github.com/jessfraz/dockerfiles
# https://docs.gitlab.com/ee/ci/docker/using_docker_build.html
# https://stackoverflow.com/questions/49874828/can-i-use-dockerfiles-in-gitlab-ci-yml
# https://codeblog.dotsandbrackets.com/continuous-integration-deployment/
# https://thecodingmachine.io/continuous-delivery-on-a-dedicated-server
#

# docker build -t gekkofyre/fedora:rawhide_dekoder .
# docker tag <image_id> gekkofyre/fedora:latest
# docker tag <image_id> gekkofyre/fedora:x86_64
# docker push gekkofyre/fedora:rawhide_dekoder
# docker login --username=yourhubusername --password=yourpassword
# rawhide_dekoder: digest: sha256:b9e2a7aafd6c8408f40e9d42c0eb32fb770c47c0232787f42807c32caa2cb15e size: 2226

# Download base image and add the proper labels
FROM fedora:rawhide
LABEL maintainer="Phobos D'thorga <phobos.gekko@gekkofyre.io>"
LABEL vendor="GekkoFyre Networks"
LABEL version="1.0"
LABEL description="See our GitLab repository for more details: <https://git.gekkofyre.io/amateur-radio/small-world-deluxe/tree/develop/docker>"

# https://askubuntu.com/questions/909277/avoiding-user-interaction-with-tzdata-when-installing-certbot-in-a-docker-contai
ARG DEBIAN_FRONTEND=noninteractive

RUN dnf clean all -y && dnf update -y
RUN dnf install -y "ca-certificates.noarch"
RUN dnf groupinstall -y "Development Tools"
RUN dnf groupinstall -y "C Development Tools and Libraries"
RUN dnf groupinstall -y "RPM Development Tools"
RUN dnf install -y "cmake.x86_64" "cmake-data.noarch" "cmake-filesystem.x86_64" "cmake-rpm-macros.noarch" "extra-cmake-modules.noarch" "boost-build.noarch" "boost-devel.x86_64" "boost-static.x86_64" "libzstd-devel.x86_64" "lz4-devel.x86_64" "snappy-devel.x86_64" "zlib-static.x86_64" "zlib-devel.x86_64" "hamlib-devel.x86_64" "libicu-devel.x86_64" "libcurl-devel.x86_64" "gflags-devel.x86_64" "mingw-w64-tools.x86_64" "qt5-qtbase-devel.x86_64" "qt5-qtbase-static.x86_64" "qt5-qtbase.x86_64" "qt5-qtbase-gui.x86_64" "qt5-qtcharts-devel.x86_64" "git.x86_64" "leveldb-devel.x86_64" "qt5-qtbase.i686" "qt5-qtbase-devel.i686" "qt5-qtbase-gui.i686" "wget.x86_64" "curl.x86_64"
RUN dnf install -y "mingw64-qt5-qtbase.noarch" "mingw64-qt5-qtbase-static.noarch" "mingw64-qt5-qt3d.noarch" "mingw64-qt5-qmake.x86_64" "mingw64-qt5-qtsvg.noarch" "mingw64-qt5-qttools.noarch" "mingw64-qt5-qtcharts.noarch" "mingw64-qt5-qtwebkit.noarch" "mingw64-qt5-qtsensors.noarch" "mingw64-qt5-qmldevtools.x86_64" "mingw64-qt5-qtbase-devel.x86_64" "mingw64-qt5-qtmultimedia.noarch" "mingw64-qt5-qtserialport.noarch" "mingw64-qt5-qtdeclarative.noarch" "mingw64-qt5-qtxmlpatterns.noarch" "mingw64-qt5-qttools-tools.x86_64" "mingw64-qt5-qtgraphicaleffects.noarch"
RUN dnf clean all -y
RUN strip --remove-section=.note.ABI-tag /usr/lib64/libQt5Core.so.5