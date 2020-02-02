[TOC]

# Small World Deluxe

|              |                           Develop                            |                            Master                            |
| :----------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
|  **Status**  | [![pipeline status](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/develop/pipeline.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/develop) | [![pipeline status](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/master/pipeline.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/master) |
| **Coverage** | [![coverage report](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/develop/coverage.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/develop) | [![coverage report](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/badges/master/coverage.svg)](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/commits/master) |

As a software project just only recently borne mid-July, 2019, `Small World` is a 'new age' [Morse Code](https://en.wikipedia.org/wiki/Morse_code) interpreter for amateur radio enthusiasts and listeners alike. It is particularly aimed at those who simply do not wish to expend the time to learn Morse Code themselves or simply wish to have the convenience of a software application undertaking the decoding of an audio stream for them.

In the near future, we wish to implement *pattern recognition techniques* into `Small World` so that it may interpret audio streams that contain even a significant amount of noise, as a good pair of ears would almost otherwise do so. We believe that by doing this, we will be going a step higher than most Morse decoding programs in the past have done so and even in the present.

------

#### Binaries

At the time of writing this (August 4th, 2019), we expect to be providing binaries for `Small World` within the coming weeks as the project slowly becomes more realized. It is not worth providing such currently since the project has only just recently been commissioned and there's very little to see, other than a proof-of-concept.

Be sure to check back often for further updates!

#### Installation / Compilation

##### Linux and similar

To begin with, you will need to install the following dependencies along with the [GCC Toolchain](https://gcc.gnu.org/):

- `CMake`
- `Boost C++` [ static and multithreaded libraries ]
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

Once you have downloaded these dependencies with the appropriate package manager, whether that be `apt-get`, `yum`, `dnf`, `pacman`, or something else entirely, you are ready to begin the compilation process! You will need to secondly download the source repository: `git clone https://git.gekkofyre.io/amateur-radio/small-world-deluxe.git`

Then `cd small-world-deluxe` before executing `mkdir build` and going into that directory too, where you'll finally perform a `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..`

Once the operation has finished, the last two commands are `make -j 8` and `make install` before you are finally able to run the application!

##### Microsoft Windows 8/10

~~Compilation should be straightforward given the above instructions and the fact that we have provided a [CMake](https://cmake.org/) build file~~. We've now learned from experience that compiling anything under Microsoft Windows is a grim undertaking that should only be performed by the best prepared of us. If you are encountering difficulties of any kind, please [download a binary release](https://git.gekkofyre.io/amateur-radio/small-world-deluxe/-/releases) for this platform instead.

#### GitLab Runner & CI/CD

In order to aid with Continuous Integration and the use of GitLab Runner with our Morse Code project for the greater Amateur Radio community, we have created our own, custom Docker image that we have uploaded to [Docker Hub](https://hub.docker.com/) and made public for all. It is under the [GekkoFyre repository](https://hub.docker.com/u/gekkofyre) and aptly tagged as [GekkoFyre/Fedora](https://hub.docker.com/r/gekkofyre/fedora), if you wish to make use of it for compiling/forking this project.

You can be rest assured that we'll keep this Docker image updated as need be.

------

#### How may I contribute to the project?

You may contribute to `Small World` in a number of ways, and they don't all necessarily require you to be a computer programmer of skill either!

Currently, we require listeners with good hearing and/or knowledge of Morse Code themselves to test the program itself out in the real world, to see how it performs in its present state.

Last, but not least, we also require individuals who are just simply happy to test the application at all. Your feedback on how it performs is absolutely invaluable to us! Any and all comments are significantly appreciated.

Although if you do have skills with regard to computer programming and would like to contribute said skills, we'd be only too happy to take them onboard! Please get in touch with the lead author, [Phobos D'thorga](mailto:phobos.gekko@gekkofyre.io), for any enquiries.

***At the moment though, the program is not in a workable state. We will advise on this page once testing of Small World can begin as it becomes more usable, bit-by-bit. We thank you for your understanding!***

#### Who are the authors?

Please refer to the files, `AUTHORS` and `CREDITS`.

#### What programming language is Small World written in?

At present, it is largely written in `C++` with small amounts of `C` here and there. The libraries themselves might contain other bits from differing languages though, mind you :)

#### Is there a release date?

There is no firm date for when `Small World` may be released as a 'stable product' and at this point in time, we simply wish to work on the project in our free time and see what happens.

------

Copyright © 2006 to 2019 – GekkoFyre Networks, All Rights Reserved.