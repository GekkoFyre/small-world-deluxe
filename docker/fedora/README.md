# GekkoFyre/Fedora

### About

This docker image was created for the [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) project, in order to assist with Continuous Integration (CI/CD) via [GitLab Runner](https://docs.gitlab.com/runner/). The following, x86_64 development libraries are installed onto a standard [Fedora Docker image](https://hub.docker.com/_/fedora):

- `CMake`
- `Boost C++` Libraries
- `ZSTD` Library
- `LZ4` Library
- `Snappy` Library
- `ZLIB`
- `HamLib`
- `ICU` Libraries
- `Curl` Libraries
- `GFlags` Libraries
- `MinGW` Tools
- `Qt5` Libraries
- `Git` Toolset
- `LevelDB` Database Libraries

There might be a few others that are not listed here, particularly dependencies which are required for compilation of C/C++ development files such as the [GCC toolchain](https://gcc.gnu.org/). But please forward any questions and/or comments towards the [Small World Deluxe](https://git.gekkofyre.io/amateur-radio/small-world-deluxe) project at our [GitLab homepage](https://git.gekkofyre.io/) which is hosted by [GekkoFyre Networks](https://gekkofyre.io/), thanks :)

#### Extra Notes

We've had to add this extra command for compatibility reasons and you or your organization may find this beneficial too:

```strip --remove-section=.note.ABI-tag /usr/lib64/libQt5Core.so.5```

If you wish to view the `Dockerfile` used to generate the image(s) in its entirety, you may do so [by clicking here](https://git.gekkofyre.io/amateur-radio/small-world-deluxe/tree/develop/docker/fedora/). We at [GekkoFyre Networks](https://gekkofyre.io/) believe in transparency for both our clients and users of any of our services.

#### Tags

|    Unique Tag     | Alternate #1 | Alternate #2 |
| :---------------: | :----------: | :----------: |
| `rawhide_dekoder` |   `latest`   |   `x86_64`   |

#### Author(s)

This Docker image was created by the following individuals:

[Phobos Aryn'dythyrn D'thorga](https://drake.network/@phobos_dthorga) (phobos[dot]gekko[at]gekkofyre[dot]io)

------

Copyright © 2006 to 2019 – GekkoFyre Networks, All Rights Reserved.