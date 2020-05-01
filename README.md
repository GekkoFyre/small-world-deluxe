[TOC]

# Small World Deluxe

|              |                           Develop                            |                            Master                            |
| :----------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
|  **Status**  | [![pipeline status](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/develop/pipeline.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/develop) | [![pipeline status](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/master/pipeline.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/master) |
| **Coverage** | [![coverage report](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/develop/coverage.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/develop) | [![coverage report](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/master/coverage.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/master) |

As a software project just only recently borne mid-July, 2019, `Small World` is a 'new age' weak-signal digital communicator powered by low bit-rate, digital voice codecs originally meant for [telephony](https://en.wikipedia.org/wiki/Telephony). Typical usage requires a radio transceiver with [SSB support](https://en.wikipedia.org/wiki/Single-sideband_modulation) and a [personal computer](https://en.wikipedia.org/wiki/Personal_computer) with a capable sound-card. Said computer must also be powerful enough to be running a modern [operating system](https://en.wikipedia.org/wiki/Operating_system) that is still supported with regular updates.

In the near future, we wish to implement *pattern recognition techniques* into `Small World` so that it may interpret audio streams that contain even a significant amount of noise, as a good pair of ears would almost otherwise do so. We believe that by doing this, we will be going a step higher than most programs of this sort have done in the past, and even in the present.

#### Current/Planned Features

Following is a short list of features, both planned and partially already implemented in some fashion, that we have for this project. We are always taking on new ideas, paradigms, and/or perspectives so please, we invite you to contribute towards `Small World Deluxe` in any manner that you are able towards.

- The ability to use highly compressed, yet rich with decently sounding audio output with regards to voice, audio codecs originally meant for telephony OTA (i.e. over-the-air).
  - This will allow digital communication primarily on the shortwave frequencies over great distances to far away places in the world from your own location, with excellent error correction features and so on.
- A functioning spectrograph / waterfall that will give you a highly detailed view of current signaling conditions, both outgoing (TX) and incoming (RX). We are [focusing a great deal of effort on this feature](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/issues/3) so far and hope to have a working version soon!
- The ability to record/playback [WAV](https://en.wikipedia.org/wiki/WAV)/[MP3](https://en.wikipedia.org/wiki/MP3)/[OGG](https://xiph.org/vorbis/)/etc files.
- A dialog rich with customization options and settings that you can configure to your heart's desire.
- Easily send messages to others throughout the world and efficiently make sense of the information you receive in-turn.
- Convenient and easy-to-use dialog windows that are made with the [Qt5 project](https://www.qt.io/developers), which is the standard within the computing industry for cross-platform software applications.
- As hinted at just above, this software application is cross-platform with excellent support for both [Linux](https://ubuntu.com/) and [Microsoft Windows 7 through to 10](https://www.microsoft.com/).
  - There is planned support for [Microsoft Windows XP](https://www.microsoft.com/) and [Macintosh OS/X](https://www.apple.com/macos), so stay tuned!

##### Screenshots

Please note that what is displayed just below may/perhaps be awfully out-of-date. If you want to see how `Small World Deluxe` currently looks, then we advise you to [download the latest release](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/-/releases), or if one is not available, then to please wait and until a sufficient version *is* available. The *lack of a release* means that we're not stable enough yet.

![smallworld_mainwindow_1](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/raw/31826c3b962ddb12429aa6f62a0b3885ef836436/assets/images/screenshots/smallworld_mainwindow_1.png)

![smallworld_settingsdialog_1](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/raw/31826c3b962ddb12429aa6f62a0b3885ef836436/assets/images/screenshots/smallworld_settingsdialog_1.png)

![smallworld_settingsdialog_2](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/raw/31826c3b962ddb12429aa6f62a0b3885ef836436/assets/images/screenshots/smallworld_settingsdialog_2.png)

![smallworld_settingsdialog_3](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/raw/31826c3b962ddb12429aa6f62a0b3885ef836436/assets/images/screenshots/smallworld_settingsdialog_3.png)

![smallworld_settingsdialog_4](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/raw/31826c3b962ddb12429aa6f62a0b3885ef836436/assets/images/screenshots/smallworld_settingsdialog_4.png)

------

#### Binaries

At the time of writing this (February 8th, 2020), we expect to be providing binaries for `Small World Deluxe` within the coming weeks/months as the project slowly becomes further realized. It is not worth providing such currently since the project has only just recently been commissioned and there's very little to see, other than a proof-of-concept.

Be sure to check back often for further updates!

#### Installation / Compilation

##### Linux and similar

To begin with, you will need to install the following dependencies along with the [GCC Toolchain](https://gcc.gnu.org/) or [LLVM/Clang](https://clang.llvm.org/):

- `CMake` [ [Build process managerial software](https://cmake.org/) ]
- `Boost C++` [ [development libraries](https://www.boost.org/) that must be *static* **and also** *multithreaded* libraries, unless you modify CMake's instructions ]
- `libusb-devel` [ [development libraries](https://github.com/libusb/libusb) ]
- `libusb-compat` [ [compatibility libraries](https://github.com/libusb/libusb-compat-0.1) ]
- `leveldb-devel` [ [development libraries](https://github.com/google/leveldb) for NoSQL information storage [by Google](https://www.google.com/) ]
- `HamLib` C++ [ [development libraries](https://hamlib.github.io/) for communicating with amateur radio transceiver rigs ]
- `Qt5` [ [development libraries](https://www.qt.io/) ]
- `Ogg Vorbis`  [audio codec libraries](https://xiph.org/vorbis/) along with the related `Opus` libraries
- `codec2` [ [Orthogonal Frequency Division Multiplexed (OFDM) modem](https://github.com/drowe67/codec2/blob/master/README_ofdm.txt) for [HF SSB](https://en.wikipedia.org/wiki/Single-sideband_modulation) ]
- `Qwt` [ [graphing libraries](https://qwt.sourceforge.io/) for the instrumental display of information ]
- `FFTW` [ [libraries for computing the discrete Fourier transform (DFT)](http://fftw.org/) in one or more dimensions ]

And for Linux-based systems in particular, you will further require the following:

* `libilmbase-dev`
* `libopenexr-dev`
* `libgstreamer1.0-dev`

Once you have downloaded these dependencies with the appropriate package manager, whether that be `apt-get`, `yum`, `dnf`, `pacman`, or something else entirely, you are ready to begin the compilation process! You will need to secondly download the source repository: `git clone https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe.git`

Then `cd small-world-deluxe` before executing `mkdir build` and going into that directory too, where you'll finally perform a `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..`

Once the operation has finished, the last two commands are `make -j$(nproc)` and `make install` before you are finally able to run the application!

##### Microsoft Windows 8/10

If you wish to compile your own binaries for the more recent releases of Microsoft Windows (8/10), since those are the only officially supported versions of said operating system ([we support Linux too!](#linux-and-similar)), you will have to make use of [MinGW](http://www.mingw.org/) or even possibly [Cygwin](https://www.cygwin.com/) for the compilation process.

This is due to the fact that we use `codec2` for our [OFDM modem of choice](https://en.wikipedia.org/wiki/Orthogonal_frequency-division_multiplexing) with regards to communications, and it itself cannot be compiled with [Microsoft Visual Studio](https://visualstudio.microsoft.com/) from our experiences. The authors do not seem to support it in any capacity at least with MinGW being the official route of choice for Microsoft Windows.

Speaking as the author of Small World Deluxe, compilation proceeds completely okay with a recently updated MinGW installation provided you have all the requisite libraries installed. But even if you are unsure of everything required being installed or not, then CMake/GCC does an alright job at letting you know what is left.

#### GitLab Runner & CI/CD

In order to aid with Continuous Integration and the use of GitLab Runner with our weak-signal project for the greater [Amateur Radio](https://en.wikipedia.org/wiki/Amateur_radio) community, we will soon be [creating our own Docker image(s) that we'll be uploading to the code repository](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/container_registry) here at [GekkoFyre Networks](https://gekkofyre.io/). This is a newer feature that has been recently introduced to [GitLab](https://gitlab.com/) and we hope to make use of it as soon as possible!

You can be rest assured that we'll keep this Docker image updated as need be.

------

#### How may I contribute to the project?

You may contribute to `Small World Deluxe` in a number of ways, and they don't all necessarily require you to be a computer programmer of skill either!

Currently, we require listeners with good hearing and/or knowledge themselves to test the program itself out in the real world, to see how it performs in its present state.

Last, but not least, we also require individuals who are just simply happy to test the application at all. Your feedback on how it performs is super invaluable to us! Any and all comments are significantly appreciated.

Although if you do have skills with regard to computer programming and would like to contribute said skills, we'd be only too happy to take them onboard! Please get in touch with the lead author, [Phobos D'thorga](mailto:phobos.gekko@gekkofyre.io), for any enquiries.

***At the moment though, the program is only in a semi-workable state. We will advise on this page once testing of Small World Deluxe can be recommended and/or begin as the application itself  becomes more usable, bit-by-bit. We thank you for your understanding!***

#### Who are the authors?

Please refer to the files, `AUTHORS` and `CREDITS`.

#### What programming language is Small World written in?

At present, it is largely written in `C++` with small amounts of `C` here and there. The libraries themselves might contain other bits from differing languages though, mind you :)

For the build script, so that you may boot-load `makefiles` into your choice of IDE, whether that be [Microsoft Visual Studio](https://visualstudio.microsoft.com/), [CLion](https://www.jetbrains.com/clion/), `nmake`, plain ol' `make`, or something else, we make use of the application, [CMake](https://cmake.org/)! Which is invaluable to us for cross-platform compilation.

We also use `YAML` for CI/CD with [GitLab Runner](https://docs.gitlab.com/runner/), for example.

#### Is there a release date?

There is no firm date for when `Small World` may be released as a 'stable product' and at this point in time, we simply wish to work on the project in our free time and see what happens.

#### Verified operating systems

We have verified, `Small World Deluxe`, to be working okay on the following operating systems:

- [Microsoft Windows 10](https://en.wikipedia.org/wiki/Windows_10) Professional & Home
- [Linux](https://en.wikipedia.org/wiki/Linux)
  - [Linux Mint](https://en.wikipedia.org/wiki/Linux_Mint) 19.2 Cinnamon
  - Kernel: <= `4.15.0`

Please remember that this is not a complete list!

#### Unsupported operating systems

The following operating systems are currently unsupported at this stage:

- [Apple Macintosh OS/X](https://en.wikipedia.org/wiki/MacOS)

Support for these will be provided in the near future, but please remember to check back often so as to see the progress we have made!

------

Copyright © 2006 to 2020 – [GekkoFyre Networks](https://gekkofyre.io/), All Rights Reserved.