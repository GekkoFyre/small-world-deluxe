# GekkoFyre/Archlinux

### About

This docker image was created for the [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) project, in order to assist with Continuous Integration (CI/CD) via [GitLab Runner](https://docs.gitlab.com/runner/). The following, x86_64 development libraries are installed onto a heavily modified Arch Linux image, with the two authors being [haffmans](https://hub.docker.com/u/haffmans) and [myself](https://drake.network/@phobos_dthorga). Along with [Qt5](https://www.qt.io/developers/) being the prominent cross-compilation application on this Docker image (Linux -> Microsoft Windows), the following may or may not also exist:

- `CMake`
- `Boost C++` Libraries
- `ZSTD` Library
- `LZ4` Library
- `ZLIB`
- `HamLib`
- `GFlags` Libraries
- `MinGW` Tools
- `Git` Toolset
- `LevelDB` Database Libraries
- `libusb` Enumeration & I/O Libraries
- `libpng` Lossless Imaging Libraries
- `libjpeg-turbo` Lossy Imaging Libraries
- `libtiff` Photographic Libraries
- `FFTW` Library for Fast Fourier Transformations
- `PortAudio` Audio Device Enumeration & I/O Library
- `OpenCV` Real-time Computer Vision Library

There might be a few others that are not listed here, particularly dependencies which are required for compilation of C/C++ development files such as the [GCC toolchain](https://gcc.gnu.org/). But please forward any questions and/or comments towards the [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) project at our [GitLab homepage](https://git.gekkofyre.io/) which is hosted by [GekkoFyre Networks](https://gekkofyre.io/), thanks :)

#### Extra Notes

If you wish to view the `Dockerfile` used to generate the image(s) in its entirety, you may do so [by clicking here](https://git.gekkofyre.io/amateur-radio/small-world-deluxe/tree/develop/docker/arch/). We at [GekkoFyre Networks](https://gekkofyre.io/) believe in transparency for both our clients and users of any of our services.

The MinGW image we have created in this instance is [based off](https://hub.docker.com/r/haffmans/mingw-qt5) a specialized, [Arch Linux](https://www.archlinux.org/) image made by the author, [haffmans](https://hub.docker.com/u/haffmans). We'd like to extend our thanks towards them for providing their content to the greater public, as it's especially difficult to find cross-compilation resources when going from Linux -> Microsoft Windows. Or at least, it is in our experience.

It should lastly be noted that many of the packages mentioned above under the [About heading](#About) are of the `MinGW-w64` system architecture type. Again, this is for cross-building reasons. If you wish to use this image for building regular, Linux-based images then we advise you to look elsewhere, thank you.

#### Information on Tags

|      |   **Original Container**   | **Cotainer #2** | **Container #3** |
| :-------------: | :----------: | :----------: | :----------: |
|    **Tag**    |    `latest`     |   `mingw`   |    `smallworld`    |
| **Source** | [haffmans/mingw-qt5:latest](haffmans/mingw-qt5:latest) (modified) | `latest` (modified) |    `mingw`    |

#### Tag: `latest`

These are the system statistics as outputted by `yay -Ps --noconfirm` at the end of Docker container creation:

```bash
Yay version v9.2.1
===========================================
Total installed packages: 406
Total foreign installed packages: 1
Explicitly installed packages: 113
Total Size occupied by packages: 5.4 GiB
===========================================
Ten biggest packages:
mingw-w64-gcc: 731.9 MiB
wine: 490.0 MiB
go: 476.3 MiB
linux-firmware: 464.8 MiB
mingw-w64-qt5-base: 183.4 MiB
mingw-w64-icu: 150.5 MiB
python: 144.5 MiB
gcc: 139.0 MiB
mingw-w64-crt: 135.1 MiB
gcc-libs: 116.4 MiB
===========================================
phobosdthorga@PhobosVM:~/Docker/archlinux/latest$
```

#### Tag: `mingw`

As per the same notes just above, here are the system statistics for this Docker container:

```bash
Yay version v9.2.1
===========================================
Total installed packages: 408
Total foreign installed packages: 2
Explicitly installed packages: 115
Total Size occupied by packages: 5.5 GiB
===========================================
Ten biggest packages:
mingw-w64-gcc: 731.9 MiB
wine: 490.0 MiB
go: 476.3 MiB
linux-firmware: 464.8 MiB
mingw-w64-qt5-base: 183.4 MiB
mingw-w64-icu: 150.5 MiB
python: 144.5 MiB
gcc: 139.0 MiB
mingw-w64-crt: 135.1 MiB
gcc-libs: 116.4 MiB
===========================================
phobosdthorga@PhobosVM:~/Docker/archlinux/mingw$
```

#### Tag: `smallworld`

This is the final Docker container and the one meant to be used for building Small World Deluxe when cross-compiling from Linux -> Microsoft Windows:

```bash
Yay version v9.2.1
===========================================
Total installed packages: 423
Total foreign installed packages: 5
Explicitly installed packages: 123
Total Size occupied by packages: 6.2 GiB
===========================================
Ten biggest packages:
mingw-w64-gcc: 731.9 MiB
wine: 490.0 MiB
go: 476.3 MiB
linux-firmware: 464.8 MiB
mingw-w64-boost: 350.1 MiB
mingw-w64-opencv: 272.4 MiB
mingw-w64-qt5-base: 183.4 MiB
mingw-w64-icu: 150.5 MiB
python: 144.5 MiB
gcc: 139.0 MiB
===========================================
phobosdthorga@PhobosVM:~/Docker/archlinux/smallworld$
```

#### Author(s)

This particular Docker image was created by the following individuals:

[Phobos Aryn'dythyrn D'thorga](https://drake.network/@phobos_dthorga) (phobos[dot]gekko[at]gekkofyre[dot]io)

------

Copyright © 2006 to 2019 – GekkoFyre Networks, All Rights Reserved.