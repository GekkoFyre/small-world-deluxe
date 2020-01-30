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
# https://docs.docker.com/engine/reference/builder/#/user
# https://hub.docker.com/r/haffmans/mingw-qt5
# https://aur.archlinux.org/packages/mingw-w64-boost/
# https://aur.archlinux.org/packages/mingw-w64-opencv/
#
# sudo docker build -t gekkofyre/archlinux:latest .
# sudo docker login --username=yourhubusername --password=yourpassword
# sudo docker push gekkofyre/archlinux:latest
# latest: digest: sha256:98a9c7d138a1d2b9d2438d0e3e35e9baf12bfa97c66cffaee5b40d5a50028ae6 size: 6617

# Download base image and add the proper labels
FROM haffmans/mingw-qt5:latest
LABEL maintainer="Phobos D'thorga <phobos.gekko@gekkofyre.io>"
LABEL vendor="GekkoFyre Networks"
LABEL version="1.0"
LABEL description="See our GitLab repository for more details: <https://git.gekkofyre.io/amateur-radio/small-world-deluxe/tree/develop/docker>"

# Refresh local `pacman` database
RUN pacman -Syy --noconfirm

# Install the `rankmirrors` plugin firstly
RUN pacman -S --noconfirm "pacman-contrib" "ca-certificates"

# https://wiki.archlinux.org/index.php/mirrors#Sorting_mirrors
# Rank the mirrors by fastest to slowest
RUN cp -v /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist.backup
RUN awk '/^## United States$/{f=1}f==0{next}/^$/{exit}{print substr($0, 2)}' /etc/pacman.d/mirrorlist.backup
RUN rankmirrors -n 6 /etc/pacman.d/mirrorlist.backup > /etc/pacman.d/mirrorlist

# Once the mirrors have been ranked, clean up and refresh the cache once more before doing an upgrade on the system
RUN pacman -Syy --noconfirm
RUN pacman -Syu --noconfirm

RUN pacman -S --noconfirm "libtool" "perl" "flex" "gperf" "ruby" "unzip" "automake" "autoconf" "cmake" "patch" "bison" "intltool" "zlib" "icu" "curl" "wget" "git" "texinfo" "bzip2" "python2" "base-devel" "pacutils" "sudo" "go" "libuhd" "libusb-compat" "libxml2" "lua" "python" "tcl" "swig" "python-mako" "doxygen" "sed"

# Enable the `multilib` package repository
RUN wget -O - https://git.gekkofyre.io/amateur-radio/small-world-deluxe/raw/develop/docker/arch/latest/enable_multilib.sh | bash

# Refresh the database after adding the `multilib` package repository
RUN pacman --sync --refresh --noconfirm
RUN pacman -Syy --noconfirm
RUN pacman -Syu --noconfirm

# Install `wine` now that `multilib` has been added and refreshed
RUN pacman -S --noconfirm "wine"

# Create the build-user
RUN useradd builduser -m

# Delete the build-user's password
RUN passwd -d builduser

# Allow the build-user passwordless sudo
RUN printf 'builduser ALL=(ALL) ALL\n' | tee -a /etc/sudoers

# Now, run the command that will compile and install `yay`
RUN sudo -u builduser bash -c 'cd ~ && git clone https://aur.archlinux.org/yay.git && cd yay && makepkg -si --noconfirm'

# Cleanup any unneeded temporary files on the system
RUN yay -Sc --noconfirm
RUN pacman -Sc --noconfirm

# Print system statistics before finally signing-off!
RUN yay -Ps --noconfirm