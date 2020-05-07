[TOC]

## PortAudio / PortAudioCpp

### Compilation

#### MinGW & Microsoft Windows

You will firstly want to clone the latest source code of [PortAudio](http://portaudio.com/) from its [Git repository](https://app.assembla.com/spaces/portaudio/git/source). Once you have done so, navigate towards the root of the source directory with the `MinGW 64-bit shell` before entering the folder, `bindings`. There you will want to delete the sub-folder, `cpp`, whereupon it will be replaced with the adjustments and custom-made `CMakeLists.txt` from our own repository specifically made for PortAudio and its C++ bindings, located within, `src/contrib/portaudio/cpp`, of Small World Deluxe.

The issues we found with the C++ bindings for PortAudio is that they were not compiling successfully all the time under `MinGW` for Microsoft Windows and in the cases where it did compile, then [Small World Deluxe](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe) wouldn't correctly link with the libraries generated thereafter, nor any other C/C++ project for that matter. So we made our own adjustments and therefore provided them to you and the wider opensource community for greater benefit. It seems that the bindings in question have not been updated for years by the PortAudio team, and have been forgotten, at the time of writing this.

But in the same `MinGW` 64-bit shell as before, you need to create a `build` directory within `bindings/cpp` before `cd`'ing into it and then executing, `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..`, and provided that everything executes okay, you will need to next run `make`. This can be done by performing the command, `make -j 8`.

An install target for `make` has not been created at this stage yet by [Phobos D'thorga](https://code.gekkofyre.io/phobos-dthorga) (the author of Small World Deluxe) so you should find two files which are named (or at least similarly so) to, `libportaudiocpp_static.a` and `libportaudio.a`, within the build directory which need copying to a specific location. The location is, `mingw64/lib`, and you will need to also copy, `include/portaudiocpp`, towards, `mingw64/include`, otherwise the compilation of Small World Deluxe will not proceed correctly.

If you wish to 'uninstall' PortAudio and its C++ bindings from your `MinGW` setup, simply delete the two aforementioned files along with its `include` directory, and you are good to go! Otherwise we are happy to answer any questions and/or issues you may have at the [Git repository for Small World Deluxe](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe/-/issues).

#### Notes about ASIO support with non-Microsoft (MSVC) compilers on Windows

Please note that the following text below has been pulled directly from [the documentation that is provided with the PortAudio project](https://app.assembla.com/spaces/portaudio/git/source/master/src/hostapi/asio/ASIO-README.txt) itself. We at [GekkoFyre Networks](https://gekkofyre.io/) did not author this and therefore ascertain no responsibility.

> [Steinberg](https://www.steinberg.net/en/company/developers.html) did not specify a calling convention in the IASIO interface definition. This causes the Microsoft compiler to use the proprietary `thiscall` convention which **is not** compatible **with** other compilers, such as compilers from **Borland** (**BCC** and **C++Builder**) and **GNU** (**gcc**). Steinberg's ASIO SDK will compile but crash on initialization if compiled with a **non-Microsoft compiler** on Windows.
>
> PortAudio solves this problem **using** the `iasiothiscallresolver` library which is included in the distribution. When building ASIO support for non-Microsoft compilers, be sure to compile and link with `iasiothiscallresolver.cpp`. Note that `iasiothiscallresolver` includes conditional directives which cause it to have no effect if it is compiled with a Microsoft compiler, or on the Macintosh.
>
> If you use `configure` and `make` (see above), this should be handled automatically for you.
>
> For further information about the IASIO `thiscall` problem **see this page**: http://www.rossbencina.com/code/iasio-thiscall-resolver

------

Copyright © 2006 to 2020 – [GekkoFyre Networks](https://gekkofyre.io/), All Rights Reserved.