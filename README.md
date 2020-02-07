[TOC]

# Small World Deluxe

|              |                           Develop                            |                            Master                            |
| :----------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
|  **Status**  | [![pipeline status](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/develop/pipeline.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/develop) | [![pipeline status](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/master/pipeline.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/master) |
| **Coverage** | [![coverage report](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/develop/coverage.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/develop) | [![coverage report](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/master/coverage.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/master) |

As a software project just only recently borne mid-July, 2019, `Small World` is a 'new age' weak-signal digital communicator powered by low bit-rate, digital voice codecs originally meant for [telephony](https://en.wikipedia.org/wiki/Telephony). Typical usage requires a radio transceiver with [SSB support](https://en.wikipedia.org/wiki/Single-sideband_modulation) and a [personal computer](https://en.wikipedia.org/wiki/Personal_computer) with a capable sound-card. Said computer must also be powerful enough to be running a modern [operating system](https://en.wikipedia.org/wiki/Operating_system) that is still supported with regular updates.

In the near future, we wish to implement *pattern recognition techniques* into `Small World` so that it may interpret audio streams that contain even a significant amount of noise, as a good pair of ears would almost otherwise do so. We believe that by doing this, we will be going a step higher than most programs of this sort have done in the past, and even in the present.

------

#### Binaries

At the time of writing this (February 8th, 2020), we expect to be providing binaries for `Small World Deluxe` within the coming weeks/months as the project slowly becomes further realized. It is not worth providing such currently since the project has only just recently been commissioned and there's very little to see, other than a proof-of-concept.

Be sure to check back often for further updates!

#### Installation / Compilation

##### Linux and similar

To begin with, you will need to install the following dependencies along with the [GCC Toolchain](https://gcc.gnu.org/) or [LLVM/Clang](https://clang.llvm.org/):

- `CMake`
- `Boost C++` [ these must be *static* **and also** *multithreaded* libraries, unless you modify CMake's instructions ]
- `libzstd-devel`
- `lz4-devel`
- `zlib-devel`
- `libusb-devel`
- `leveldb-devel`
- `HamLib` C++ [ development libraries ]
- `Qt5` [ development libraries ]

And for Linux-based systems in particular, you will further require the following:

* `libilmbase-dev`
* `libopenexr-dev`
* `libgstreamer1.0-dev`

Once you have downloaded these dependencies with the appropriate package manager, whether that be `apt-get`, `yum`, `dnf`, `pacman`, or something else entirely, you are ready to begin the compilation process! You will need to secondly download the source repository: `git clone https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe.git`

Then `cd small-world-deluxe` before executing `mkdir build` and going into that directory too, where you'll finally perform a `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..`

Once the operation has finished, the last two commands are `make -j$(nproc)` and `make install` before you are finally able to run the application!

##### Microsoft Windows 8/10

~~Compilation should be straightforward given the above instructions and the fact that we have provided a [CMake](https://cmake.org/) build file~~. We've now learned from experience that compiling anything under Microsoft Windows is a grim undertaking that should only be performed by the best prepared of us. If you are encountering difficulties of any kind, please [download a binary release](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/-/releases) for this platform instead.

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