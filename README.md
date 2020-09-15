[TOC]

# Small World Deluxe

|            |                         Experimental                         |                           Develop                            |                            Master                            |
| :--------: | :----------------------------------------------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
| **Status** | [![pipeline status](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/badges/experimental/pipeline.svg)](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/commits/experimental) | [![pipeline status](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/badges/develop/pipeline.svg)](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/commits/develop) | [![pipeline status](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/badges/master/pipeline.svg)](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/commits/master) |

As a software project just only semi-recently borne mid-July, 2019, `Small World Deluxe` is an ultra-modern weak-signal digital communicator powered by low bit-rate, digital voice codecs such as the highly regarded and open-source [Codec2](https://en.wikipedia.org/wiki/Codec_2), with others like [FT8](https://physics.princeton.edu/pulsar/K1JT/wsjtx.html) and [related](https://www.jtdx.tech/) to come within the near future. Typical usage requires a radio transceiver with [SSB support](https://en.wikipedia.org/wiki/Single-sideband_modulation) and a [personal computer](https://en.wikipedia.org/wiki/Personal_computer) with a capable sound-card. Said computer must also be powerful enough to be running a modern [operating system](https://en.wikipedia.org/wiki/Operating_system) that is still supported with regular updates ([see requirements below](#General-Requirements)).

Having been written from the ground-up with the C++ programming language and the [Qt5 Project](https://www.qt.io/) set of libraries, which is the standard within the computing industry for cross-platform software applications that make use of a GUI, this project is designed for ease-of-use by both computer programmer and casual amateur radio user alike. We have also implemented the [Hamlib libraries](https://hamlib.github.io/) so that our users have the easiest time possible configuring their radios with `Small World Deluxe`.

We are presently implementing support for [Codec2](https://github.com/drowe67/codec2) as the modem of choice at this stage so that we may then begin implementing more advanced features much more quickly and easily. We don't shy away from the fact that we rely on libraries developed by others, where possible; why re-invent the wheel when the work has already been done for you?

If you wish to contribute your own work and effort to the project then by all means, please do so! This is 100% a group effort where everyone interested takes part, which is why we made this project free and open-source in nature. We want the entire amateur radio community to benefit from the fruits of our labor and then some.

#### Current / Planned Features

Following is a short list of features, both planned and partially already implemented in some fashion, that we have for this project. We are always taking on new ideas, paradigms, and/or perspectives so please, we invite you to contribute towards `Small World Deluxe` in any manner that you are able towards.

- The ability to send textual messages to others around the globe over the HF bands, thanks to support with the aforementioned `Codec2` modem. Basic support for this is already present to a certain degree, and the user may choose to compile `Small World Deluxe` with or without the `Codec2` library.
  - This will allow digital communication primarily on the shortwave frequencies over great distances to far away places in the world from your own location, with excellent error correction features and so on.
- A functioning spectrograph / waterfall that will give you a highly detailed view of current signaling conditions, both outgoing (TX) and incoming (RX). There is already a basic operating version of this present, we just need to amp up the resolution along with performance and we'll have an excellent spectrogram feature to boot!
- The ability to record/playback [WAV](https://en.wikipedia.org/wiki/WAV)/[OGG](https://xiph.org/vorbis/)/[Opus](https://opus-codec.org/)/etc files. These are not meant for real-time communications due to the inherent latency issues therein of these codecs and the requirement for a high-performance computer to keep up.
- A settings dialog rich with customisation options that you can configure to your heart's desire. Reasonably decent support for this is already present.
- Easily send messages to others throughout the world and efficiently make sense of the information you receive in-turn.
- As hinted at just above, this software application is cross-platform with excellent support for both [Linux](https://ubuntu.com/) and [Microsoft Windows 7 through to 10](https://www.microsoft.com/). Although for `Microsoft Windows`, you must compile the program with [MinGW](http://www.mingw.org/) or [Cygwin](https://www.cygwin.com/) due to `Codec2` having no support with [Microsoft Visual Studio](https://visualstudio.microsoft.com/).
  - There is planned support for legacy operating systems such as [Microsoft Windows XP](https://www.microsoft.com/) and perhaps earlier, along with other such systems like [Macintosh OS/X](https://www.apple.com/macos) where there is no current implemented support, so stay tuned!
- Make extensive use of the [PortAudio project](http://www.portaudio.com/) for easy cross-compatibility with audio/multimedia devices between `Linux`, `Macintosh OS/X`, and `Microsoft Windows` systems. The support for this within `Small World Deluxe` is largely established already.

##### Screenshots

Please note that what is displayed just below may/perhaps be awfully out-of-date. If you want to see how `Small World Deluxe` currently looks, then we advise you to [download the latest release](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/releases), or if a binary is not available, then to please wait and until a sufficient version *is* available. The *lack of a release* means that we're not stable enough yet.

![smallworld_mainwindow_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/f72a0eb986abe8c94fb755b2457da0ace7ac281c/assets/images/screenshots/2020-07-15/Screenshot_20200715_001410.png)

![smallworld_settingsdialog_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/f72a0eb986abe8c94fb755b2457da0ace7ac281c/assets/images/screenshots/2020-07-15/Screenshot_20200715_001457.png)

![smallworld_settingsdialog_2](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/f72a0eb986abe8c94fb755b2457da0ace7ac281c/assets/images/screenshots/2020-07-15/Screenshot_20200715_001532.png)

![smallworld_settingsdialog_3](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/f72a0eb986abe8c94fb755b2457da0ace7ac281c/assets/images/screenshots/2020-07-15/Screenshot_20200715_001557.png)

![smallworld_settingsdialog_4](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/f72a0eb986abe8c94fb755b2457da0ace7ac281c/assets/images/screenshots/2020-07-15/Screenshot_20200715_001645.png)

![smallworld_aboutdialog_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/f72a0eb986abe8c94fb755b2457da0ace7ac281c/assets/images/screenshots/2020-07-15/Screenshot_20200715_001707.png)

------

#### Binaries

At the time of writing this (17th August, 2020), we expect to be providing binaries for `Small World Deluxe` within the coming weeks/months as the project slowly becomes further realized. It is not worth providing such currently since the project has only just recently been commissioned and there's very little to see, other than a proof-of-concept.

Be sure to check back often for further updates!

#### Installation / Compilation

##### General Requirements

This is to come soon! Check back often for updates :)

##### Linux and similar

To begin with, you will need to install the following dependencies along with the [GCC Toolchain](https://gcc.gnu.org/) or [LLVM/Clang](https://clang.llvm.org/):

- `CMake` [ [build process managerial software](https://cmake.org/) ]
- `Boost C++` [ [development libraries](https://www.boost.org/) that must be *static* **and also** *multithreaded* libraries, unless you modify CMake's instructions ]
- `NVIDIA CUDA Toolkit` [ [parallel computing platform](https://developer.nvidia.com/cuda-zone) for general computing on GPUs (optional) ]
- `PortAudio` [ [multimedia libraries](http://portaudio.com/) ]
- `Codec2` [ [next generation low bit-rate speech codec for amateur radio use](https://github.com/drowe67/codec2) ]
- `libusb-devel` [ [development libraries](https://github.com/libusb/libusb) ]
- `leveldb-devel` [ [development libraries](https://github.com/google/leveldb) for NoSQL information storage [by Google](https://www.google.com/) ]
- `HamLib` C++ [ [development libraries](https://hamlib.github.io/) for communicating with amateur radio transceiver rigs ]
- `Qt5` [ [development libraries](https://www.qt.io/) ]
- `Ogg Vorbis`  [audio codec libraries](https://xiph.org/vorbis/) along with the related `Opus` libraries
- `Qwt` [ [graphing libraries](https://qwt.sourceforge.io/) for the instrumental display of information ]
- `Zlib` [ [data compression library](https://zlib.net/) ]
- `zstd` [ [real-time compresion algorithm](https://facebook.github.io/zstd/) ]
- `iconv` [ [converts between different character encodings](https://www.gnu.org/software/libiconv/) ]

Some of the aforementioned dependencies need to be manually cloned via their respective Git repositories (links provided above) and compiled by hand, due to the fact that we are working with bleeding edge technology and code. The dependencies we are aware of which fall into this camp are the following:

- `Codec2`
- `PortAudio`
- `Hamlib` (and the `C++ bindings`)

Hints on how to compile this software can be taken from the `.gitlab-ci.yml` file within the root of our Git repository, for where we are in deficient provisioning of such documentation for. We cannot stress enough that binaries will be provided soon, it's just that `Small World Deluxe` is not quite at that stage yet of being demonstrated on a larger scale.

Once you have downloaded and/or compiled the aforementioned dependencies with the appropriate package manager or dev-tools, whether that be `apt-get`, `yum`, `dnf`, `pacman`, or something else entirely, you are ready to begin the compilation process of `Small World Deluxe` itself! You will firstly need to download the source repository though: `git clone https://code.gekkofyre.io/amateur-radio/small-world-deluxe.git`

Then `cd small-world-deluxe` before executing `mkdir build` and going into that directory too, where you'll finally perform a `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..`

Once the operation has finished, the last two commands are `make -j$(nproc)` and `make install` before you are finally able to run the application!

##### Compilation on Microsoft Windows 8/10 with MinGW

At the present stage, due to the fact the only modem we support is `Codec2`, and if you build `Small World Deluxe` without said library, the application will effectively be useless for its intended role, we only support compilation on Microsoft Windows with `MinGW` or `Cygwin`. This is due to the fact that `Codec2` is not supported under `Microsoft Visual Studio` in any condition. It also doesn't matter whether you perform a possible `MinGW` compilation straight from a `Microsoft Windows` host or from a `Linux` distribution, as an example.

Speaking as the author of Small World Deluxe, compilation proceeds completely okay with a recently updated MinGW installation provided you have all the requisite libraries installed. But even if you are unsure of everything required being installed or not, then CMake/GCC does an alright job at letting you know what is left.

Requirements for such a build as this are [listed above](#General-Requirements) and if there's anything we've possibly left out, then we'd sincerely appreciate hearing about it, thank you!

##### Notes for users of Microsoft Windows

We will be providing binaries for this software application soon enough when we feel there is something worthwhile to demonstrate to the public, otherwise for now, `Small World Deluxe` is just an experiment. But that does not mean it will *remain* as an experiment, we fully expect this application to mature into a full fledged software release, with features comparable to other such amateur radio programs like [JTDX](https://www.jtdx.tech/).

Once you have successfully compiled Small World Deluxe, you may need to copy over some shared DLLs towards the directory where the executable is located, in order to successfully open the application without errors and/or crashing. You can determine what specific DLLs are required by using a program such as [Dependency Walker](http://www.dependencywalker.com/), which is extremely useful for this particular task.

Because `Dependency Walker` is a third-party program, we do not provide any support as such nor are we affiliated with their authors and/or coders in any way. We simply like the tools they have freely provided (at the time of writing) and highly recommend them in their current form.

#### GitLab Runner & CI/CD

In order to aid with Continuous Integration and the use of [GitLab Runner](https://docs.gitlab.com/runner/) with our weak-signal project for the greater [Amateur Radio](https://en.wikipedia.org/wiki/Amateur_radio) community, we will soon be [creating our own Docker image(s) that we'll be uploading to the code repository](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/container_registry) here at [GekkoFyre Networks](https://gekkofyre.io/). This is a newer feature that has been recently introduced to [GitLab](https://gitlab.com/) and we hope to make use of it as soon as possible!

You can be rest assured that we'll keep this Docker image updated as need be.

------

#### Miscellaneous Notes & Troubleshooting

##### Receiving error(s) that's related to something about a `locale`?

If you receive an error that's similar to, `Setting the locale has failed!`, or even possibly, `locale::facet::_S_create_c_locale name not valid`, then the locale that has been configured for your system of choice may not be set correctly. [This particular article from Stack Overflow](https://stackoverflow.com/a/10236868/4293625) may help you in your endeavors but if not, you will have to [open an issue with the Small World Deluxe team over at their Git repository](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues), where we will be happy to help you and your particular system of choice.

##### Help! I'm encountering issues with making a connection between `Small World Deluxe` and my radio transceiver rig!

This could be due to the individual settings of your radio itself. For example, I had to modify the CAT TOT (timeout time) for my [Yaesu FT-450D](https://www.yaesu.com/indexVS.cfm?cmd=DisplayProducts&ProdCatID=102&encProdID=870B3CA7CFCB61E6A599B0EFEA2217E4&DivisionID=65&isArchived=0) before a connection could be reliably made between this application and the rig itself, so something similar might have to be done for your use-case, if encountering such problems. It's no fault of SWD at all, but simply down to the differences of each radio rig out there :)

If you have a particularly stubborn problem regarding this, then we'd be glad to help you out to the best of our ability within the [Issues section of this code repository](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues). You might also benefit from reading [the Hamlib FAQ](https://github.com/Hamlib/Hamlib/wiki/FAQ) as well, since this software is powered by it in some respects.

##### I'm being asked to contribute 'crash reports'? What's this and why am I being asked?

This is a very good question. Our application reports on such metrics so that we may better develop `Small World Deluxe` that tailors to the needs of our users. You are given the choice of whether to consent to this or not upon initial program start, and then the ability to change said choice later on by navigating to the Setting's Dialog.

![smallworld_ask_consent_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-08-17/Screenshot_20200817_125947.png)

We feel that this tailors to most user's privacy needs although we'd really appreciate to hear all and any feedback on this. It is the community surrounding Small World Deluxe that drives the development in general.

##### How may I contribute to the project?

You may contribute to `Small World Deluxe` in a number of ways, and they don't all necessarily require you to be a computer programmer of skill either!

Currently, we require listeners with good hearing and/or knowledge themselves to test the program itself out in the real world, to see how it performs in its present state.

Last, but not least, we also require individuals who are just simply happy to test the application at all. Your feedback on how it performs is super invaluable to us! Any and all comments are significantly appreciated.

Although if you do have skills with regard to computer programming and would like to contribute said skills, we'd be only too happy to take them onboard! Please get in touch with the lead author, [Phobos D'thorga](mailto:phobos.gekko@gekkofyre.io), for any enquiries.

***At the moment though, the program is only in a semi-workable state. We will advise on this page once testing of Small World Deluxe can be recommended and/or begin as the application itself  becomes more usable, bit-by-bit. We thank you for your understanding!***

##### Who are the authors?

Please refer to the files, `AUTHORS` and `CREDITS`, with the latter being a collection of who's responsible for writing the libraries we make use of. We try and keep this file up-to-date as much as possible but we are only Human (to some degree!).

##### What programming language is Small World written in?

At present, it is largely written in `C++` with sprinklings of `C` here and there. Any scripts responsible for building this application are both largely and preferrably written with [CMake](https://cmake.org/) for maximum portability across a multitude of system architectures and operating systems. The libraries that `Small World Deluxe` depends upon might contain other bits from differing languages though, mind you, with `Fortran` being a big one for radio-related code :)

We also use `YAML` extensively for controlling CI/CD-related actions with [GitLab Runner](https://docs.gitlab.com/runner/), for example.

##### Is there a release date?

There is no firm date for when `Small World Deluxe` may be released as a 'stable product' and at this point in time, we simply wish to work on the project in our free moments and see what happens.

##### Verified operating systems

We have verified, `Small World Deluxe`, to be working okay on the following operating systems:

- x86_64
  - [Microsoft Windows 10](https://en.wikipedia.org/wiki/Windows_10) Professional & Home
    - MinGW (April, 2020)
  - [Linux](https://en.wikipedia.org/wiki/Linux)
    - [Arch Linux](https://www.archlinux.org/) (August, 2020)
    - [Linux Mint](https://en.wikipedia.org/wiki/Linux_Mint) 19.2 Cinnamon
    - [Linux Mint](https://en.wikipedia.org/wiki/Linux_Mint) [20 Cinnamon "Ulyana"](https://linuxmint.com/edition.php?id=281) & [KDE](https://kde.org/)
    - [Manjaro KDE Linux 20.0.1](https://manjaro.org/)
    - Kernel: <= `4.15.0` & `5.4.0-42-generic`

Please remember that this is not a complete list!

##### Unsupported operating systems

The following operating systems are currently unsupported at this stage:

- [Apple Macintosh OS/X](https://en.wikipedia.org/wiki/MacOS)

Support for these will be provided in the near future, but please remember to check back often so as to see the progress we have made!

------

Copyright © 2006 to 2020 – [GekkoFyre Networks](https://gekkofyre.io/), All Rights Reserved.
