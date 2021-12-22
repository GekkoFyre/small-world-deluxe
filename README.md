[TOC]

Last updated: 22nd December, 2021

# Small World Deluxe

|            |                         Experimental                         |                           Develop                            |                            Master                            |
| :--------: | :----------------------------------------------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
| **Status** | [![pipeline status](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/badges/experimental/pipeline.svg)](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/commits/experimental) | [![pipeline status](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/badges/develop/pipeline.svg)](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/commits/develop) | [![pipeline status](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/badges/master/pipeline.svg)](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/commits/master) |

As a mixed ideology software project borne mid-July, 2019, `Small World Deluxe` is an ultra-modern weak-signal digital communicator powered by low bit-rate, digital voice codecs such as the highly regarded and open-source [Codec2](https://en.wikipedia.org/wiki/Codec_2), with others like [FT8](https://physics.princeton.edu/pulsar/K1JT/wsjtx.html) and [related](https://www.jtdx.tech/) to come within the near future. Typical usage requires a radio transceiver with [SSB support](https://en.wikipedia.org/wiki/Single-sideband_modulation) and a [personal computer](https://en.wikipedia.org/wiki/Personal_computer) with a capable sound-card. Said computer must also be powerful enough to be running a modern [operating system](https://en.wikipedia.org/wiki/Operating_system) that is still supported with regular updates ([see requirements below](#General-Requirements)).

Having been written from the ground-up with the C++ programming language and the [Qt Project](https://www.qt.io/) as the framework of choice, which is the standard within the computing industry for cross-platform software applications that make use of a GUI, this project is designed for ease-of-use by both computer programmer and casual amateur radio user alike. We have also implemented the [Hamlib libraries](https://hamlib.github.io/) so that our end-users have the easiest time possible configuring their radios with `Small World Deluxe`.

If you wish to contribute your own work and effort to the project then by all means, please do so! This is 100% a group effort where everyone interested takes part, which is why we made this project free and open-source in nature. We want the entire amateur radio community to benefit from the fruits of our labor and then some.

You may also [contribute to our Ko-fi](https://ko-fi.com/gekkofyre) if you are unable to offer programming skills, but still desire to contribute in some manner. Another suggestion is personal artwork and/or graphical design contributions, as we are always in need of designers who can especially work their magic when it comes to vector graphics. But raster is needed too, just not as much as those wonderful vector graphics!

#### Current / Planned Features

Following is a short list of features, both planned and partially already implemented in some fashion, that we have for this project. We are always taking on new ideas, paradigms, and/or perspectives so please, we invite you to contribute towards `Small World Deluxe` in any manner that you are able towards. Should you wish to suggest something, we recommend that you [make a post on our Issues board](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues)!

- The ability to send textual messages to others around the globe over the HF bands, thanks to support with the aforementioned `Codec2` modem, as well as others. Basic support for `Codec2` is already present to a certain degree, and the user may choose to compile `Small World Deluxe` with or without this library as according to their choosing.
  - This will allow digital communication primarily on the shortwave frequencies over great distances to far away places in the world from your own location, with excellent [ECC features](https://en.wikipedia.org/wiki/Error_correction_code) and so on.
- Communicate with other end-users over the Internet via XMPP, transparently from within `Small World Deluxe`. Already, there is an [Instant Messaging (IM)](https://en.wikipedia.org/wiki/Instant_messaging) component coded up in a semi-mature form and it is continually being improved upon as one of the most major features of our software project.
  - Sign-up, login, message archiving/history, file transfers, password reset, etc. are all handled excellently and transparently from within `Small World Deluxe` itself.
  - Within the future, we'll also add the ability for end-users who practice in the hobby of amateur radio to share certain things such as audio links, spectrograms, rig control, etc. with other end-users over this built-in IM client. The sky's the limit!
  - Because we are running our own servers regarding XMPP connectivity, at our own cost, we have also allowed for the ability for third-party clients to connect. Plus it's XMPP, that's one of the main features of the protocol! Interoperability.

- A functioning spectrograph / waterfall that will give you a highly detailed view of current signaling conditions, for both outgoing (TX) and incoming (RX) signals.
  - There is already a basic operating version of this present, we just need to make a few more final touches and we'll be done!
- A world map/atlas you can choose your ham-shack location from and henceforth, rather easily, share the geographical co-ordinates with others such as via the aforementioned XMPP connectivity or over HF transmission.
  - Basic support for this is already provided, we simply need to perform more work on this feature so it's both stable and useful.

- The ability to record/playback [WAV](https://en.wikipedia.org/wiki/WAV)/[OGG](https://xiph.org/vorbis/)/[Opus](https://opus-codec.org/)/etc files.
  - These are not meant for real-time communications due to the inherent latency issues therein of these codecs and the requirement for a high-performance computer to keep up.
- A settings dialog rich with customization options that you can configure to your heart's desire.
  - Reasonably decent support for this is already present.

- As hinted at just above, this software application is cross-platform with excellent support for both [Linux](https://ubuntu.com/) and [Microsoft Windows 7 through to 10](https://www.microsoft.com/). Although for `Microsoft Windows`, you must compile the program with [MinGW](http://www.mingw.org/) or [Cygwin](https://www.cygwin.com/) due to `Codec2` possessing no support with [Microsoft Visual Studio](https://visualstudio.microsoft.com/).
  - There is planned support for legacy operating systems such as [Microsoft Windows XP](https://www.microsoft.com/) and perhaps earlier, along with other such systems like [Macintosh OS/X](https://www.apple.com/macos) where there is no current implemented support, so stay tuned!
- We are currently well underway with regards to implementing a rich audio subsystem thanks to the [cross-platform OpenAL-soft API](https://github.com/kcat/openal-soft).
  - Support for this is already implemented, we just need to start using those features in more places!


##### Screenshots

Please note that what is displayed just below may perhaps be awfully out-of-date. If you want to see how `Small World Deluxe` currently looks, then we advise you to [download the latest release](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/releases), or if a binary is not available, then to please wait and until a sufficient version *is* available. The *lack of a release* simply means that we're not stable enough yet for public dissertation.

![smallworld_mainwindow_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_192845.png)

![smallworld_settingsdialog_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_192854.png)

![smallworld_settingsdialog_2](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_192908.png)

![smallworld_settingsdialog_3](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_192926.png)

![smallworld_settingsdialog_4](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_192943.png)

![smallworld_aboutdialog_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_193004.png)

![smallworld_aboutdialog_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_193013.png)

![smallworld_aboutdialog_1](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/raw/develop/assets/images/screenshots/2020-11-14/Screenshot_20201113_193022.png)

------

#### Binaries

At the time of writing this (22nd December, 2021), we expect to be providing binaries for `Small World Deluxe` within 6-12 months as the project slowly matures and becomes further realized. It is not worth providing any binaries currently since the project is in a very early and experimental state, and we fear with all the bugs present too due to those two aforementioned things, it will lead to premature dissatisfaction amongst our target users.

You must remember as well that it takes quite a bit more time to code an application in C/C++ than to do the equivalent in another, similar language such as [Python](https://www.python.org/), which is much higher-level! Generally speaking, the lower level you go, the more man-hours and boilerplate are required to accomplish the same, given task.

Be sure to check back often for further updates!

#### Installation / Compilation

##### General Requirements

We have an [extremely well maintained Wiki](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/wikis/home) primarily thanks to the efforts of [Phobos D'thorga (VK3VKK)](https://code.gekkofyre.io/phobos-dthorga), whom is also the original author of `Small World Deluxe`. Please see [Compilation & Dependencies](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/wikis/Setup/Compilation-&-Dependencies) for the complete requirements and instructions on how to compile `Small World Deluxe` for yourself, either on Linux-based operating systems or those of Microsoft Windows.

Speaking as of now (22nd December 2021), Apple/Macintosh operating systems are not supported at least and until I can find the help of another individual who will kindly assist me with the testing of `Small World Deluxe` on such, since I do not own *any* Apple hardware or known software in my life.

Although on the thought of compiling `Small World Deluxe` for yourself, we strongly recommend that you wait and until a binary is available. It is not an easy process despite the use of `CMakeLists.txt`  and our excellent documentation, simply because of all the intertwining dependencies we use.

##### Notes for end-users of Microsoft Windows

Once you have successfully compiled `Small World Deluxe`, you may need to copy over some shared DLLs towards the directory where the executable is located, in order to successfully open the application without errors and/or crashing. You can determine what specific DLLs are required by using a program such as [Dependency Walker](http://www.dependencywalker.com/), which is extremely useful for this particular task.

Because `Dependency Walker` is a third-party program, we do not provide any support as such nor are we affiliated with their authors and/or coders in any way. We simply like the tools they have freely provided (at the time of writing) and highly recommend them in their current form.

------

#### Miscellaneous Notes & Troubleshooting

##### Receiving error(s) that's related to something about a `locale`?

If you receive an error that's similar to, `Setting the locale has failed!`, or even possibly, `locale::facet::_S_create_c_locale name not valid`, then the locale that has been configured for your system of choice may not be set correctly. [This particular article from Stack Overflow](https://stackoverflow.com/a/10236868/4293625) may help you in your endeavors but if not, you will have to [open an issue with the Small World Deluxe team over at their Git repository](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues), where we will be happy to help you and your particular system of choice.

##### Help! I'm encountering issues with making a connection between `Small World Deluxe` and my radio transceiver rig!

This could be due to the individual settings of your radio itself. For example, I had to modify the CAT TOT (timeout time) for my [Yaesu FT-450D](https://www.yaesu.com/indexVS.cfm?cmd=DisplayProducts&ProdCatID=102&encProdID=870B3CA7CFCB61E6A599B0EFEA2217E4&DivisionID=65&isArchived=0) before a connection could be reliably made between this application and the rig itself, so something similar might have to be done for your use-case, if encountering such problems. It's no fault of SWD at all, but simply down to the differences of each radio rig out there :)

If you have a particularly stubborn problem regarding this, then we'd be glad to help you out to the best of our ability within the [Issues section of this code repository](https://code.gekkofyre.io/amateur-radio/small-world-deluxe/-/issues). You might also benefit from reading [the Hamlib FAQ](https://github.com/Hamlib/Hamlib/wiki/FAQ) as well, since this software is powered by it in some respects.

##### I'm being asked to contribute 'crash reports'? What's this and why am I being asked?

This is a very good question. Our application reports on such metrics so that we may better develop `Small World Deluxe` that tailors to the needs of our end-users. You are given the choice of whether to consent to this or not upon initial program start, and then the ability to change said choice later on by navigating to the Setting's Dialog.

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

Copyright © 2006 to 2022 – [GekkoFyre Networks](https://gekkofyre.io/), All Rights Reserved.
