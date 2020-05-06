[TOC]

## PortAudio / PortAudioCpp

### Compilation

#### MinGW & Microsoft Windows

You will firstly want to clone the latest source code of [PortAudio](http://portaudio.com/) from its [Git repository](https://app.assembla.com/spaces/portaudio/git/source). Once you have done so, navigate towards the root of the source directory with the MinGW 64-bit shell and execute, `./configure --enable-static --enable-shared --with-winapi=asio`, before also executing, `make -j 8 && make install`, as the final commands.

You will then want to copy over the directory, `src/contrib/portaudio/cpp`, from [Small World Deluxe's](https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe) own repository after firstly deleting the directory, `bindings/cpp`, within the PortAudio code you downloaded earlier with Git, before replacing entirely with our own adjustments.

In the same MinGW 64-bit shell as before, you need to create a `build` directory within `bindings/cpp` before `cd`'ing into it and then executing, `cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..`, and provided that everything executes okay, you will need to next run `make`. This can be done the same as per above by running, `make -j 8`.

An install target for `make` has not been created at this stage for the C++ bindings so a file which is named (or at least similarly so), `libportaudiocpp.a`, within the build directory you created earlier, should be copied towards, `mingw64/lib`, of your MinGW installation. You will also need to manually copy the `include/portaudiocpp` folder towards `mingw64/include` otherwise the compilation of Small World Deluxe will not proceed correctly.