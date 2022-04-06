#### Introduction

We readily admit that [Small World Deluxe](https://code.gekkofyre.io/amateur-radio/small-world-deluxe) uses a large and wide variety of third-party libraries and tools. It is a sincere belief of ours that if a large amount of programming time can be saved by the usage of a third-party library and therefore by not 'reinventing the wheel', so to say, then we'll readily seek out one to fill that gap. This has many advantages, particularly including a rapid development style of coding.

Please check back to this page often as we are always updating it, through the removal and addition of various dependencies, as well as new and more thoroughly updated installation instructions. Additions for other, various `*nix` distros will be included over time too. If you wish for a particular operating system and/or distro to be included, then please [feel free to open an issue within our Issue Tracker](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues).

#### Microsoft Windows (MinGW via MSYS2)

The instructions for performing a compilation with [MSYS2](https://www.msys2.org/) under Microsoft Windows are more-or-less finished at this stage. It is the author's preference of this Wiki that the MinGW (i.e. [MSYS2](https://www.msys2.org/) in this case) development platform is used where possible, especially over instances where [Cygwin](https://www.cygwin.com/) could be used. This is due to the fact that `Small World Deluxe` has been optimized from the ground-up for MSYS2 on Microsoft Windows systems.

* `base-devel`
* `mingw-w64-x86_64-toolchain`
* `mingw-w64-x86_64-kdeclarative-qt5`
* `mingw-w64-x86_64-uasm`
* `icu-devel`
* `cmake`
* `mingw-w64-x86_64-extra-cmake-modules`
* `zlib-devel`
* `openssl-devel`
* `git`
* `mingw-w64-x86_64-snappy`
* `mingw-w64-x86_64-libusb-compat-git `
* `mingw-w64-x86_64-git-lfs`
* `mingw-w64-x86_64-hidapi`
* `mingw-w64-x86_64-libvorbis`
* `mingw-w64-x86_64-opus`
* `mingw-w64-x86_64-libopusenc`
* `mingw-w64-x86_64-qxmpp`
* `mingw-w64-x86_64-enchant`
* `mingw-w64-x86_64-aria2`
* `mingw-w64-x86_64-ffmpeg`
* `mingw-w64-x86_64-taglib`
* `mingw-w64-x86_64-qwt-qt5`
* `mingw-w64-x86_64-sonnet-qt5`

Lastly, you will need to install all the required libraries for [Qt Project](https://www.qt.io/), and they too are listed below this paragraph. It is upon the `Qt` libraries that the foundation of `Small World Deluxe` is built upon; the GUI, all the glue code, etc. so we are in debt to its contributors for their hard, selfless work. Some libraries are likely not needed from the list below, but we have included them anyhow to be extra sure that the end-user is not left out on any missing dependencies. Onto the list!

- `mingw-w64-x86_64-qt5`
- `mingw-w64-x86_64-qt5-3d`
- `mingw-w64-x86_64-qt5-base`
- `mingw-w64-x86_64-qt5-charts`
- `mingw-w64-x86_64-qt5-connectivity`
- `mingw-w64-x86_64-qt5-datavis3d`
- `mingw-w64-x86_64-qt5-declarative`
- `mingw-w64-x86_64-qt5-imageformats`
- `mingw-w64-x86_64-qt5-multimedia`
- `mingw-w64-x86_64-qt5-networkauth`
- `mingw-w64-x86_64-qt5-quick3d`
- `mingw-w64-x86_64-qt5-quickcontrols`
- `mingw-w64-x86_64-qt5-quickcontrols2`
- `mingw-w64-x86_64-qt5-quicktimeline`
- `mingw-w64-x86_64-qt5-remoteobjects`
- `mingw-w64-x86_64-qt5-script`
- `mingw-w64-x86_64-qt5-scxml`
- `mingw-w64-x86_64-qt5-serialbus`
- `mingw-w64-x86_64-qt5-serialport`
- `mingw-w64-x86_64-qt5-location`
- `mingw-w64-x86_64-qt5-speech`
- `mingw-w64-x86_64-qt5-svg`
- `mingw-w64-x86_64-qt5-tools`
- `mingw-w64-x86_64-qt5-translations`
- `mingw-w64-x86_64-qt5-websockets`
- `mingw-w64-x86_64-qt5-winextras`

If you require the *debug* libraries for performing development work on the source code of `Small World Deluxe`, then you will additionally require the following packages. As of November 5th, 2021, this list is up-to-date for `MSYS2`:

* `mingw-w64-x86_64-qt5-debug`
* `mingw-w64-x86_64-qt5-3d-debug`
* `mingw-w64-x86_64-qt5-base-debug`
* `mingw-w64-x86_64-qt5-charts-debug`
* `mingw-w64-x86_64-qt5-connectivity-debug`
* `mingw-w64-x86_64-qt5-datavis3d-debug`
* `mingw-w64-x86_64-qt5-declarative-debug`
* `mingw-w64-x86_64-qt5-imageformats-debug`
* `mingw-w64-x86_64-qt5-multimedia-debug`
* `mingw-w64-x86_64-qt5-networkauth-debug`
* `mingw-w64-x86_64-qt5-quick3d-debug`
* `mingw-w64-x86_64-qt5-quickcontrols-debug`
* `mingw-w64-x86_64-qt5-quickcontrols2-debug`
* `mingw-w64-x86_64-qt5-quicktimeline-debug`
* `mingw-w64-x86_64-qt5-remoteobjects-debug`
* `mingw-w64-x86_64-qt5-script-debug`
* `mingw-w64-x86_64-qt5-scxml-debug`
* `mingw-w64-x86_64-qt5-serialbus-debug`
* `mingw-w64-x86_64-qt5-serialport-debug`
* `mingw-w64-x86_64-qt5-speech-debug`
* `mingw-w64-x86_64-qt5-svg-debug`
* `mingw-w64-x86_64-qt5-tools-debug`
* `mingw-w64-x86_64-qt5-websockets-debug`
* `mingw-w64-x86_64-qt5-winextras-debug`

Once you have compiled [Boost C++](#compilation-of-boost-c-under-mingw-via-msys2), [Hamlib](#compilation-of-hamlib-under-mingw-via-msys2), [KDE Marble](#kde-marble-geographical-mapping-coordinates), and then [Codec2](#compilation-of-codec2-under-mingw-via-msys2) for the [MSYS2](https://www.msys2.org/) subsystem, you may proceed with the compilation and installation of `Small World Deluxe` itself! [CMake](https://cmake.org/) is required for this operation and we recommend that you create a separate `build` directory for the compilation (as shown below). Once the dependencies have all been set up, you only need to execute the following commands under an MSYS2 shell, from within the root of the `Small World Deluxe` project directory:

```bash
sh bootstrap.sh
mkdir build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
mingw32-make -j$(nproc)
mingw32-make install
```

You will then be able to find all the required files within the 'build' folder, but otherwise, feel free to contact us if you have any issues!

------

##### Compilation of `Boost C++` libraries under MinGW via MSYS2

It seems that the [Boost C++ libraries](https://www.boost.org/) which are part of MSYS2's repositories are not directly compatible with `Small World Deluxe` and as such, you will have to compile this set of libraries by hand instead. But not to worry! The process is very simple with the provided instructions ahead.

You will firstly have to [download the latest version](https://www.boost.org/) of these libraries (preferably in the [7-Zip format](https://www.7-zip.org/) if possible) before extracting the source code to your 'Home' directory (i.e. `C:\msys64\home\${username}\` as an example), where upon you'll then launch `mingw64.exe` from the root directory of your MSYS2 installation. Run the following commands, in sequence, as they complete within the terminal window:

```bash
cd boost_${version}
sh bootstrap.sh
./b2 install --prefix=/mingw64 toolset=gcc address-model=64 instruction-set=native link=static threading=multi runtime-link=shared variant=release --build-type=complete
```

That's all which is required!

##### Compilation of `Hamlib` under MinGW via MSYS2

**UPDATE**: You can simply download the MinGW binaries from [Hamlib's official website](https://hamlib.github.io/) now instead (you will wish to use the provided `gcc` library) and add the paths to your environmental variables. This will be recognized by `Small World Deluxe`'s build scripts and compilation should proceed freely! This is much easier than having to go through the alternative route below.

As mentioned above, the second, alternative route is to compile `Hamlib` yourself straight from its source code via `MSYS2` which can be better in some situations, and lead to fewer errors provided you know what you're doing. To start off, you will need to firstly download the latest version from [Hamlib's official website](https://hamlib.github.io/) and then decompress the source code archive before `cd`'ing into the said directory.  You will then need to execute the following commands in order of each other to compile and install the source code:

```bash
mkdir build && cd build
./../configure --disable-shared CFLAGS="-fdata-sections -ffunction-sections" LDFLAGS="-Wl,--gc-sections" --prefix="C:/msys64/mingw64"
mingw32-make -j$(nproc)
mingw32-make install
```

The process should be rather straightforward but if you have any issues and/or questions, then please do not hesitate to ask about them within our [official Issue Tracker](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues).

##### KDE Marble (Geographical Mapping & Coordinates)

To begin with the compilation of this library, you will need to firstly copy the source directory for `KDE Marble` itself from within the `Small World Deluxe` project's `contrib` folder, found at the path, `./src/contrib/marble`, if navigating from the parent. Once you have copied this to the personal home directory of your [MSYS2 installation](https://www.msys2.org/), you may begin with compilation by opening a shell (via launching `mingw64.exe` from the `MSYS2` root directory ideally) and then executing the commands, in sequence, found below.

```bash
cd marble
mkdir build && cd build
cmake -G "Unix Makefiles" -DQTONLY=TRUE -DQT5BUILD=TRUE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/mingw64" ..
mingw32-make -j$(nproc)
mingw32-make install
```

That's all which is required for this library! If everything proceeded without error then you are done. Otherwise, feel free to ask for help [within our Issue Tracker](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues). 

##### Compilation of `Codec2` under MinGW via MSYS2

Unlike the aforementioned `Hamlib`, there are no ready-to-use binary sources for `Codec2` and you will need to perform a compilation of the most up-to-date sources instead. This is because of how frequently `Codec2` is updated on a continual basis. Thankfully, it's not too difficult to compile and with some determination, you will be going in no time! To start with, you will need to [grab the latest sources for Codec2 from the official GitHub repository](https://github.com/drowe67/codec2).

Once the sources have been extracted into the applicable home directory of your [MSYS2 installation](https://www.msys2.org/), you will then want to open the `mingw64.exe` shell in order to make an `x86_64` compilation. You may then navigate to the root of the `Codec2` sources where you'll create a `build` directory before `cd`'ing into it, and then executing the following commands in order:

```bash
mkdir build && cd build
cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=NO -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/mingw64" ..
mingw32-make -j$(nproc)
mingw32-make install
```

If you happen to receive the following error when executing the last command:

```bash
...
...
Consolidate compiler generated dependencies of target vq_mbest
[100%] Built target vq_mbest
Install the project...
-- Install configuration: "Debug"
CMake Error at cmake/GetDependencies.cmake:14 (include):
  include could not find requested file:

    C:/msys64/home/phobo/codec2/cmake/GetPrerequisites.cmake
Call Stack (most recent call first):
  cmake_install.cmake:41 (include)


mingw32-make: *** [Makefile:144: install] Error 1

phobo@GekkoPC MINGW64 ~/codec2/build
```

Then you need to download the [following file](https://github.com/Kitware/CMake/blob/master/Modules/GetPrerequisites.cmake) towards the `./cmake` folder within the root of `Codec2`'s source directory, before attempting to execute the `mingw32-make install` command once more.

By now the operation should be complete! If you encountered any problems and/or have questions, then please open them within our [official Issue Tracker](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues), thank you.

##### Compilation of libsndfile

The last dependency you will need to compile for a MSYS2-based system is [libsndfile](https://github.com/libsndfile/libsndfile) and then you are ready to proceed with the `Small World Deluxe` project itself!

Start by copying the provided and already tested `libsndfile` source from within the `Small World Deluxe` project's `contrib` folder, found at the path, `./src/contrib/libsndfile`, if navigating from the parent. Once you have copied this to the personal home directory of your [MSYS2 installation](https://www.msys2.org/), you may begin with compilation by opening a shell (via launching `mingw64.exe` from the `MSYS2` root directory ideally) and then executing the commands, in sequence, found below.

```bash
pacman -R mingw-w64-x86_64-libsndfile mingw-w64-x86_64-python-soundfile
mkdir build && cd build
cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=YES -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/mingw64" ..
mingw32-make -j$(nproc)
mingw32-make install
```

If you are experiencing trouble or just have a general question, then please direct those towards our [official Issue Tracker](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues) where someone knowledgeable will reply as soon as possible. Otherwise, you may now progress with the compilation of `Small World Deluxe` itself!

------

#### Microsoft Windows (Native MSVC++)

Currently, compilation is no longer at all possible with MSVC++ in any shape or form. We apologize for this but it's due to the fact that we now have support for the [Codec2 libraries](https://github.com/drowe67/codec2) and they themselves do not have any plans for this compiler as of writing (September 2020). Ask them instead if you would like this to be supported, as we sure would :)

------

#### Linux (Debian-based)

If you are using a Linux-based system to compile `Small World Deluxe`, you will require the following libraries (a Debian-based distro is used in this example for the package naming nomenclature):

- `cmake`
- `cmake-data`
- `extra-cmake-modules`
- `libpthread-stubs0-dev` (or something along those lines)
- `libboost-all-dev`
- `zlib1g-dev`
- `libasound2-dev`
- `libcodec2-0.9`
- `libsnappy-dev`
- `libusb-dev`
- `libusb-1.0-0-dev`
- `libhidapi-libusb0`
- `libudev-dev`
- `udev`
- `libvorbis-dev`
- `libogg-dev`
- `libopus-dev`
- `libssl-dev`
- `libaria2-0-dev`
- `libtag1-dev`
- `libtagc0`
- `libtagc0-dev`
- `libicu-dev`
- `libhunspell-dev`
- `hunspell-en-au`
- `hunspell-en-ca`
- `hunspell-en-us`
- `hunspell-en-gb`
- `libenchant-dev`

And the following is a *recommended* list of the required packages in regards to the `Qt5` library for a Debian-based system since we use that project for the GUI and primary internals of `Small World Deluxe`. Not all of these packages will be required and we appreciate suggestions on how to clean up the list, but for now, we like to be extra sure by covering all of our bases:

- `qtcreator`
- `qtcreator-data`
- `qtcreator-doc`
- `qttools5-dev`
- `libqwt-qt5-dev`
- `libqt5svg5-dev`
- `libqt5serialbus5-dev`
- `libqt5serialport5-dev`
- `libqt5texttospeech5-dev`

Lastly, you will require the [CMake cross-platform family of tools](https://cmake.org/) to build, test, and package `Small World Deluxe`. We chose this particular software project due to the immense and easily available support for its massive variety of operating systems and system architectures, which simply isn't available with many other options out there. You will find the `CMakeLists.txt` within the root directory of the `Small World Deluxe` project itself.

Before you can finally go on towards the compilation of `Small World Deluxe` though, you must compile three, last libraries by hand since they are either not typically offered by most package managers within Linux, or are too old of version. Please see the [compilation of `libopusenc`](#compilation-of-libopusenc-for-linux-based-systems), [`QXmpp`](#compilation-of-qxmpp-for-linux-based-systems), and then [`Hamlib`](#compilation-of-hamlib-for-linux-based-systems) for for this. Before beginning the compilation of these, be sure to execute `sh bootstrap.sh` in the root of `Small World Deluxe` beforehand. This will download all the needed, extra files from all the required Git repositories.

Once the aforementioned three libraries are compiled by hand, you may begin with `Small World Deluxe`. Start by `cd`'ing into the root of the source directory before executing the following commands:

```bash
sh bootstrap.sh
mkdir build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

That's it! You may now begin using `Small World Deluxe` but please note we are very much in the pre-alpha stages right now, and there is much development going on. The look and feel of the application may change somewhat as the months wear on.

##### Compilation of `libopusenc` for Linux-based systems

This is a required dependency for if you are using `Small World Deluxe` under a Linux-based system and a manual compilation is usually required since this component is typically not found in most repository managers. Thankfully, a compilation of this library is straightforward and easy, you just need to `cd src/contrib/libopusenc/` from the root of the `Small World Deluxe` project itself, whereupon you will execute the following commands to begin compilation and then, installation:

```bash
sh autogen.sh
./configure --enable-static --enable-shared
make -j$(nproc)
sudo make install
```

And that's it! Please feel free to [open an issue with us](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues) if you have encountered any problems and require further assistance.

##### Compilation of `QXmpp` for Linux-based systems

This too is a required dependency and while it is typically provided with most package managers under Linux, unlike `libopusenc`, the versions that are easily available are too old for what `Small World Deluxe` requires and will work with. To start with, you just need to `cd src/contrib/qxmpp` from the root of the `Small World Deluxe` project itself, whereupon you will execute the following commands to begin compilation and then, installation:

```bash
sudo apt-get remove --purge libqxmpp-dev libqxmpp1 -y
mkdir build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

Now there's only one more library to compile and then we are done! Move onto the compilation of `Hamlib` before we can finally begin the building of `Small World Deluxe` at last.

##### Compilation of `Hamlib` for Linux-based systems

`Hamlib` is the last of the dependencies that are required to be compiled for Linux-based systems and again is for the same reason as `QXmpp` above; it's readily available through most package managers but the versions offered are typically too old for what's required by `Small World Deluxe` since we are creating tools that work with the most bleeding edge of software. To start with, you just need to `cd src/contrib/Hamlib` from the root of the `Small World Deluxe` project itself, whereupon you will execute the following commands to begin compilation and then, installation:

```bash
sudo apt-get remove --purge libhamlib++-dev libhamlib-dev libhamlib2++c2 libhamlib2
sh bootstrap
./configure --enable-static --enable-shared --with-xml-support --with-cxx-binding --with-libusb
make -j$(nproc)
sudo make install
```

You are now done and complete! Please let us know if we've missed anything, as we wish for this to be a complete list.

#### Linux (Arch/Pacman-based)

The instructions for Linux distributions based on [pacman](https://wiki.archlinux.org/title/pacman) (such as [Arch Linux](https://archlinux.org/) or [Manjaro Linux](https://manjaro.org/)) are quite similar to those of that based on Debian-oriented distros, only in this instance, you substitute `apt-get` for `pacman` instead, along with the packages which are unique to this environment.

We have thus far found that `Small World Deluxe` compiles best with `GCC`/`G++` within this flavour of Linux, based on our own experiences with `Manjaro Linux`. You may therefore want to configure your environment to use such for compiling in order to avoid any extraneous errors. We would very much appreciate any feedback on your experiences with `Arch Linux` and related in regards to `Small World Deluxe`, so please share for the benefit of the greater community!

It should also be noted that we do use an Arch Linux-based [Docker image](https://hub.docker.com/u/gekkofyre) for compiling `Small World Deluxe` via our [GitLab Runner](https://docs.gitlab.com/runner/) (i.e. Continuous Integration/Deployment) services and related (such as [Jenkins](https://www.jenkins.io/) for Artifactory management). It is semi-regularly kept up-to-date and might be an excellent resource if you, as the end-user, wish to reverse engineer the script otherwise until we provide further support in this area. Or in any case, public binaries for `Small World Deluxe` itself once it becomes stable enough.

Otherwise, the dependencies list we do currently have for Arch-based distributions is otherwise incomplete and only a recommendation. Please proceed with caution if you wish to go down this path! We take no responsibility.

- `icu`
- `hunspell`
- `hunspell-en_us`
- `hunspell-en_gb`
- `hunspell-en_ca`
- `hunspell-en_au`
- `nuspell`
- `enchant`
- `zstd`
- `libusb`
- `hidapi`
- `lz4`
- `zlib`
- `leveldb`
- `libusb`
- `fftw`
- `snappy`
- `qwt`
- `libogg`
- `libvorbis`
- `opus`
- `opusfile`
- `texinfo`
- `libusb-compat`
- `cmake`
- `wget`
- `aria2`
- `openal`
- `ffmpeg`
- `taglib`